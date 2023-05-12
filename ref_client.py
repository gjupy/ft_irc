import sys
import socket
import re
import threading

# Check if server and port are provided
if len(sys.argv) != 3:
    print("Usage: python irc_client.py <server> <port>")
    exit(1)

SERVER = sys.argv[1]
PORT = int(sys.argv[2])

# Create a function to send a command to the server
def send_cmd(cmd):
    client_socket.sendall(cmd.encode())

# Establish a connection
client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
try:
    client_socket.connect((SERVER, PORT))
    print(f"""Connection to IRC_trash established successfully!\n
     __          __  _
     \ \        / / | |
      \ \  /\  / /__| | ___ ___  _ __ ___   ___
       \ \/  \/ / _ \ |/ __/ _ \| '_ ` _ \ / _ \\
        \  /\  /  __/ | (_| (_) | | | | | |  __/
         \/  \/ \___|_|\___\___/|_| |_| |_|\___|


        Welcome to our IRC client!.
    """)
except ConnectionRefusedError:
    print(f"Unable to connect to the server at {SERVER}:{PORT}")
    sys.exit(1)

# Flag to indicate whether a KeyboardInterrupt has occurred
program_should_exit = threading.Event()

# Create a separate thread to receive and print server responses
def receive_responses():
    while not program_should_exit.is_set():
        try:
            resp = client_socket.recv(1024).decode()
            if not resp:
                break
            print(resp, end='', flush=True)

            # Check if the response indicates a successful channel join
            if resp.startswith("you created channel") or resp.startswith("you joined channel"):
                match = re.search(r"you created channel #([^']+)", resp)
                if match:
                    channel_name = match.group(1)[:-2]
                    print(f"""
     __          __  _
     \ \        / / | |
      \ \  /\  / /__| | ___ ___  _ __ ___   ___
       \ \/  \/ / _ \ |/ __/ _ \| '_ ` _ \ / _ \\
        \  /\  /  __/ | (_| (_) | | | | | |  __/
         \/  \/ \___|_|\___\___/|_| |_| |_|\___|


        Welcome to the #{channel_name} channel! Enjoy your stay.
    """)
        except socket.error:
            break

# Start the thread for receiving server responses
response_thread = threading.Thread(target=receive_responses)
response_thread.start()

try:
    # User input handling
    while True:
        input_cmd = input()
        if input_cmd.upper() == "QUIT":
            raise SystemExit
        else:
            send_cmd(input_cmd + '\n')
except (KeyboardInterrupt, SystemExit):
    print("\nYou have left the IRC_trash server.")
    program_should_exit.set()
    client_socket.close()
    response_thread.join()
