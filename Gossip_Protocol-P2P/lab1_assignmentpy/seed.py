import socket as sk
import threading
import logging

# Configuring logging for structured terminal output
logging.basicConfig(
    level=logging.INFO,
    format="%(asctime)s [%(levelname)s] %(message)s",
    datefmt="%Y-%m-%d %H:%M:%S"
)

LISTEN_IP = "0.0.0.0"  # IP for listening
LISTEN_PORT = int(input("Enter port number: "))  # Port for listening
connected_peers = {}  # Dictionary to hold peer address : degree mapping

def log_output_to_file(message):
    """Append the output message to a file."""
    try:
        with open("outputseed.txt", "a") as file:
            file.write(message + "\n")
    except Exception as e:
        logging.error("Failed to write output: %s", e)

def create_server_socket():
    """Create a server socket and return it."""
    try:
        s = sk.socket(sk.AF_INET, sk.SOCK_STREAM)
        logging.info("Server socket successfully created.")
        return s
    except Exception as e:
        logging.error("Error in creating socket: %s", e)
        raise

def bind_server_socket(server_socket):
    #used to bind the server socket on the defined IP and Port."""
    try:
        server_socket.bind((LISTEN_IP, LISTEN_PORT))
        logging.info("Socket bound to %s:%d", LISTEN_IP, LISTEN_PORT)
    except Exception as e:
        logging.error("Error binding socket: %s", e)
        bind_server_socket(server_socket)  # Retry binding

def peers_to_string(peers):
    """Convert peer dictionary to a comma-separated string."""
    return ",".join(peers.keys())

def handle_dead_node(msg):
    #helps in Processing dead node message and updating the connected_peers dict."""
    parts = msg.split(":")
    if len(parts) < 3:
        logging.error("Invalid dead node message: %s", msg)
        return
    _, dead_peer, informer_peer = parts
    if dead_peer in connected_peers:
        del connected_peers[dead_peer]
        logging.info("Removed dead node: %s", dead_peer)
    else:
        logging.info("Dead node %s not found.", dead_peer)
    
    if informer_peer in connected_peers:
        connected_peers[informer_peer] = max(0, connected_peers[informer_peer] - 1)
        logging.info("Reduced degree of informer node %s to %d", informer_peer, connected_peers[informer_peer])
    else:
        logging.info("Informer node %s not found.", informer_peer)

def update_peer(peer_key, new_degree):
    """Update the degree of a peer."""
    if peer_key in connected_peers:
        connected_peers[peer_key] = new_degree
        logging.info("Updated degree for %s to %d", peer_key, new_degree)
    else:
        logging.info("Peer %s not in the list.", peer_key)

def peer_dict_to_string(peer_dict):
    """Convert peer dictionary to a semicolon-separated string of addr:degree pairs."""
    return ";".join([f"{addr}:{degree}" for addr, degree in peer_dict.items()])

def increase_peer_degree(peer_key):
    """Increment the degree of the given peer."""
    if peer_key in connected_peers:
        connected_peers[peer_key] += 1
        logging.info("Incremented degree for %s to %d", peer_key, connected_peers[peer_key])
        return f"Degree incremented to {connected_peers[peer_key]}"
    else:
        logging.info("Peer %s not found for degree increment.", peer_key)
        return "Peer not found"

def process_peer_connection(conn, addr):
    #Handles all communication with a peer connection
    while True:
        try:
            data = conn.recv(1024).decode('utf-8')
            if data:
                if data.startswith("INCREMENT:"):
                    _, peer_ip, peer_port = data.split(":")
                    peer_key = peer_ip + ":" + peer_port
                    logging.info("Increment request from %s", peer_key)
                    reply = increase_peer_degree(peer_key)
                    conn.send(reply.encode('utf-8'))

                elif data.startswith("UPDATE:"):
                    logging.info("Received update message: %s", data)
                    parts = data.split(":")
                    if len(parts) >= 4:
                        _, peer_ip, peer_port, degree_str = parts
                        peer_key = peer_ip + ":" + peer_port
                        update_peer(peer_key, int(degree_str))
                        message = f"Degree update successful for {peer_key}"
                        conn.send(message.encode('utf-8'))
                    else:
                        logging.error("Invalid UPDATE message: %s", data)

                elif data.startswith("Dead Node"):
                    handle_dead_node(data)

                else:
                    parts = data.split(":")
                    if len(parts) >= 3:
                        peer_port = parts[1]
                        peer_degree = int(parts[2])
                        peer_key = addr[0] + ":" + peer_port
                        connected_peers[peer_key] = peer_degree
                        message = f"Connection received from {peer_key} with degree {peer_degree}"
                        logging.info(message)
                        log_output_to_file(message)
                        reply = peer_dict_to_string(connected_peers)
                        conn.send(reply.encode('utf-8'))
                        logging.info("Current connected peers: %s", connected_peers)
                    else:
                        logging.error("Invalid message format: %s", data)
            else:
                break
        except Exception as e:
            logging.error("Error while processing connection from %s: %s", addr, e)
            break
    conn.close()

def start_server():
    #this helps begin listening for peer connections and spawn a new thread for each connection.
    server_socket = create_server_socket()
    bind_server_socket(server_socket)
    server_socket.listen(5)
    logging.info("Seed node is now listening on %s:%d", LISTEN_IP, LISTEN_PORT)
    while True:
        conn, peer_address = server_socket.accept()
        server_socket.setblocking(1)
        thread = threading.Thread(target=process_peer_connection, args=(conn, peer_address))
        thread.start()

if __name__ == "__main__":
    start_server()