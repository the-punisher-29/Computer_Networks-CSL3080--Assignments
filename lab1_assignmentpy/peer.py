import socket
import threading
import time
import hashlib
import random
import logging
from queue import Queue

# Configuring structured logging
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S"
)

# Global configuration and state
THREAD_COUNT = 3  # three threads: one for listening, liveness, gossip check respectively
JOB_IDS = [1, 2, 3]
job_queue = Queue()

MY_IP = "0.0.0.0"
try:
    PORT = int(input("Enter listening port: "))
except Exception as exc:
    logging.error("Invalid port input")
    raise exc

seed_addresses = set()  # from config.txt
all_peer_entries = set()  # aggregated from seeds
active_peers = []         # active Peer instances
gossip_hashes = []        # list of gossip message hashes to avoid duplicates
connected_seeds = []      # seeds that we connect to

OUTPUT_FILENAME = "outputpeer.txt"


def log_to_file(message):
    """Appends structured log messages to an output file."""
    try:
        with open(OUTPUT_FILENAME, "a") as f:
            if isinstance(message, dict):
                msg = f"Address: {message['address']}, Degree: {message['degree']}\n"
            else:
                msg = f"{message}\n"
            f.write(msg)
    except IOError:
        logging.error("Failed writing to log file")


class Peer:
    def __init__(self, address):
        self.address = address
        self.failure_count = 0  # to track liveness failures


def current_time():
    return time.time()


def compute_hash(msg):
    return hashlib.sha256(msg.encode()).hexdigest()


def load_seed_config():
    """Load seed addresses from config.txt in a structured way."""
    global seeds_address_list
    try:
        with open("config.txt", "r") as f:
            seeds_address_list = f.read().strip()
            logging.info("Found seed addresses:\n%s", seeds_address_list)
    except Exception:
        logging.error("Failed to read config.txt")
    finally:
        if 'f' in locals():
            f.close()


def count_seeds():
    #Populate seed_addresses set and return count
    global seeds_address_list
    for line in seeds_address_list.splitlines():
        if line:
            parts = line.split(":")
            seed_addr = f"0.0.0.0:{parts[1]}"
            seed_addresses.add(seed_addr)
    return len(seed_addresses)


def random_indices(lower, upper, count):
    indexes = set()
    while len(indexes) < count:
        indexes.add(random.randint(lower, upper))
    return indexes


def init_socket():
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        return s
    except Exception:
        logging.error("Socket creation error")
        return None


def bind_socket(s):
    try:
        s.bind((MY_IP, PORT))
    except socket.error:
        logging.error("Socket binding error")
        bind_socket(s)


# This part of the code is crucial in maintaining a power-law distribution in the designed peer network.
# In power-law distributed networks, a few nodes (seeds) have a high number of connections,
# while many nodes have fewer connections. This helps in efficient information spread and robustness.
def update_seed_degree():
    # this functn is responsible for letting the seed nodes know that this peer
    # has increased itz degree, so they can update thier records
    my_addr = f"{MY_IP}:{PORT}"
    for seed in connected_seeds:
        try:
            s = init_socket()
            ip, port_ = seed.split(":")
            s.connect((ip, int(port_)))
            msg = f"INCREMENT:{my_addr}"
            s.send(msg.encode('utf-8'))
            reply = s.recv(1024).decode("utf-8")
            logging.info("Seed response for degree increment: %s", reply)
            s.close()
        except Exception:
            logging.error("Failed to update degree on seed %s", seed)


def process_peer_connection(conn, addr):
    # this functn process incoming connections from other peers, handling
    # new connection requests, liveness checks and gossip messages
    while True:
        try:
            raw_msg = conn.recv(1024).decode("utf-8")
            if not raw_msg:
                break
            logging.info("Received: %s", raw_msg)
            parts = raw_msg.split(":")
            if parts[0].startswith("New Connect Request From"):
                if len(active_peers) < 4:  # limit peers to 4 to control network degree
                    conn.send("New Connect Accepted".encode("utf-8"))
                    new_peer = Peer(f"{addr[0]}:{parts[2]}")
                    active_peers.append(new_peer)
                    update_seed_degree()
            elif parts[0].startswith("Liveness Request"):
                reply = f"Liveness Reply:{parts[1]}:{parts[2]}:{MY_IP}:{PORT}"
                conn.send(reply.encode("utf-8"))
            elif parts[3][:6] == "GOSSIP":
                relay_gossip(raw_msg)
        except Exception:
            break
    conn.close()


def start_listener(s):
    # this is the main entry for handling incoming connections.
    # It start a new thread for each new connection from peers.
    s.listen(5)
    logging.info("Peer started listening on %s:%s", MY_IP, PORT)
    while True:
        conn, addr = s.accept()
        s.setblocking(1)
        threading.Thread(target=process_peer_connection, args=(conn, addr)).start()


def connect_to_peers(peer_list):
    # Tries to connect to peers in the provided list and maintain active peer limit
    if not peer_list:
        logging.info("No peers available to connect")
        return
    for peer in peer_list:
        try:
            s = init_socket()
            ip, port_ = peer['address'].split(":")
            s.connect((ip, int(port_)))
            request = f"New Connect Request From:{MY_IP}:{PORT}"
            s.send(request.encode("utf-8"))
            if s.recv(1024).decode("utf-8") == "New Connect Accepted":
                active_peers.append(Peer(peer['address']))
                logging.info("Connected to peer: %s", peer['address'])
                update_seed_degree()
            else:
                logging.info("Connection to %s was rejected", peer['address'])
            s.close()
        except Exception as ex:
            logging.error("Error connecting to %s: %s", peer['address'], str(ex))
    logging.info("Total connected peers: %d", len(active_peers))


def join_top_peers(peer_entries):
    # selects top-degree peers and try to connect to maintain power-law distribition
    if not peer_entries:
        return
    total = len(peer_entries)
    attempt_count = max(total // 2, 1)
    sorted_peers = sorted(peer_entries, key=lambda p: p['degree'], reverse=True)
    selection = sorted_peers[:attempt_count]
    logging.info("Attempting connection to %d peers", len(selection))
    for p in selection:
        logging.info("Attempting peer %s with degree %d", p['address'], p['degree'])
    connect_to_peers(selection)


def merge_peer_lists(raw_list):
    # takes a raw string of peers and their degrees, process them and updates the peer list
    global all_peer_entries, MY_IP
    all_peer_entries.clear()
    entries = raw_list.split(";")
    for e in entries:
        if e:
            parts = e.split(":")
            if len(parts) >= 3:
                addr = f"{parts[0]}:{parts[1]}"
                try:
                    degree = int(parts[2])
                except Exception:
                    degree = 0
                all_peer_entries.add((addr, degree))
    # update MY_IP based on last entry
    if entries:
        last = entries[-1].split(":")
        if last:
            MY_IP = last[0]
    peer_list = [{"address": a, "degree": d} for a, d in all_peer_entries]
    self_addr = f"{MY_IP}:{PORT}"
    return [peer for peer in peer_list if peer['address'] != self_addr]


def refresh_degree_at_seeds(updated_degree):
    # sends an updated degree information to the seed nodes so they can adjust their data
    my_addr = f"{MY_IP}:{PORT}"
    for seed in connected_seeds:
        try:
            s = init_socket()
            ip, port_ = seed.split(":")
            s.connect((ip, int(port_)))
            update_msg = f"UPDATE:{my_addr}:{updated_degree}"
            s.send(update_msg.encode("utf-8"))
            response = s.recv(1024).decode("utf-8")
            logging.info("Update response from seed %s: %s", seed, response)
            s.close()
        except Exception:
            logging.error("Failed degree update for seed %s", seed)

def connect_seeds():
    # This  connects to seed nodes and retrieves peer lists.
    peer_lists = []
    for i, seed in enumerate(connected_seeds):
        try:
            s = init_socket()
            ip, port_ = seed.split(":")
            s.connect((ip, int(port_)))
            my_addr = f"{MY_IP}:{PORT}"
            s.send(f"{my_addr}:0".encode("utf-8"))
            peer_list_str = s.recv(10240).decode("utf-8")
            logging.info("Seed %d list: %s", i + 1, peer_list_str)
            peer_lists.append(peer_list_str)
            s.close()
        except Exception:
            logging.error("Error connecting to seed %d", i + 1)
    combined = ";".join(peer_lists)
    processed_list = merge_peer_lists(combined)
    for p in processed_list:
        log_to_file(p)
    join_top_peers(processed_list)
    new_degree = len(active_peers)
    logging.info("New computed degree: %d", new_degree)
    refresh_degree_at_seeds(new_degree)


def register_with_seeds():
    # This  randomly selects half of the seeds and connects to them.
    global seed_addresses, connected_seeds
    seed_list = list(seed_addresses)
    k = len(seed_list) // 2 + 1
    indices = list(random_indices(0, len(seed_list) - 1, k))
    for idx in indices:
        connected_seeds.append(seed_list[idx])
    connect_seeds()


def announce_dead(peer_addr):
    # This  informs seed nodes that a peer is dead.
    msg = f"Dead Node:{peer_addr}:{current_time()}:{MY_IP}"
    logging.info("Reporting dead node: %s", msg)
    log_to_file(msg)
    for seed in connected_seeds:
        try:
            s = init_socket()
            ip, port_ = seed.split(":")
            s.connect((ip, int(port_)))
            s.send(msg.encode("utf-8"))
            s.close()
        except Exception:
            logging.error("Failed to notify seed %s about dead node", seed)


def monitor_liveness():
    # This  checks if peers are alive and removes unresponsive ones.
    while True:
        liveness_cmd = f"Liveness Request:{current_time()}:{MY_IP}:{PORT}"
        logging.info("Sending liveness request: %s", liveness_cmd)
        for peer in active_peers.copy():
            try:
                s = init_socket()
                ip, port_ = peer.address.split(":")
                s.connect((ip, int(port_)))
                s.send(liveness_cmd.encode("utf-8"))
                reply = s.recv(1024).decode("utf-8")
                logging.info("Liveness reply from %s: %s", peer.address, reply)
                s.close()
                peer.failure_count = 0
            except Exception:
                peer.failure_count += 1
                if peer.failure_count >= 3:
                    announce_dead(peer.address)
                    active_peers.remove(peer)
        time.sleep(10)


def relay_gossip(message):
    # This  spreads received gossip messages to connected peers.
    msg_hash = compute_hash(message)
    if msg_hash in gossip_hashes:
        return
    gossip_hashes.append(msg_hash)
    logging.info("Gossip: %s", message)
    log_to_file(message)
    for peer in active_peers:
        try:
            s = init_socket()
            ip, port_ = peer.address.split(":")
            s.connect((ip, int(port_)))
            s.send(message.encode("utf-8"))
            s.close()
        except Exception:
            continue


def broadcast_gossip():
    # This  sends gossip messages periodically.
    for i in range(10):
        gossip_msg = f"{current_time()}:{MY_IP}:{PORT}:GOSSIP{i+1}"
        gossip_hashes.append(compute_hash(gossip_msg))
        for peer in active_peers:
            try:
                s = init_socket()
                ip, port_ = peer.address.split(":")
                s.connect((ip, int(port_)))
                s.send(gossip_msg.encode("utf-8"))
                s.close()
            except Exception:
                logging.error("Peer %s seems down", peer.address)
        time.sleep(5)


def create_worker_threads():
    # This  creates worker threads for handling jobs.
    for _ in range(THREAD_COUNT):
        thread = threading.Thread(target=job_executor)
        thread.daemon = True
        thread.start()


def job_executor():
    # This  executes jobs from the job queue.
    while True:
        job_type = job_queue.get()
        if job_type == 1:
            sock_instance = init_socket()
            bind_socket(sock_instance)
            start_listener(sock_instance)
        elif job_type == 2:
            monitor_liveness()
        elif job_type == 3:
            broadcast_gossip()
        job_queue.task_done()


def enqueue_jobs():
    # This puts jobs in the queue and processes them.
    for j in JOB_IDS:
        job_queue.put(j)
    job_queue.join()


if __name__ == "__main__":
    # Main execution starts here. It initializes seeds, workers, and jobs.
    load_seed_config()
    total_seeds = count_seeds()
    logging.info("Total Seeds available: %d", total_seeds)
    register_with_seeds()
    create_worker_threads()
    enqueue_jobs()
