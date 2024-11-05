import time
from typing import List

def tail(filename: str, n: int = 10, follow: bool = False) -> List[str]:
    with open(filename, 'rb') as f:
        # Move to the end of the file
        f.seek(0, 2)
        
        # Store the initial lines for the `n` tail lines
        file_size = f.tell()
        chunk_size = 1024 if file_size > 1024 else file_size
        data = []

        # Read the last `n` lines to initialize
        pos = file_size
        while len(data) < n and pos > 0:
            pos = max(0, pos - chunk_size)
            f.seek(pos)
            chunk = f.read(min(chunk_size, pos + chunk_size))
            lines = chunk.splitlines()
            data = lines + data
            if pos == 0:
                break
        # Print only the last `n` lines initially
        for line in data[-n:]:
            print(line.decode('utf-8'))

        # If follow is enabled, keep checking for new lines
        if follow:
            f.seek(0, 2)  # Go to the end again for follow
            while True:
                line = f.readline()
                if line:
                    print(line.decode('utf-8'), end="")
                else:
                    time.sleep(0.1)  # Wait before trying to read new lines

# Usage example:
filename = 'C:\\ProgramData\\opencpn\\opencpn.log'
tail(filename, n=10, follow=True)
