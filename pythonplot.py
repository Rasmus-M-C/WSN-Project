import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta

file_path = 'loglistener17.txt'  # Replace with the path to your text file
def parse_timestamp(timestamp):
    # Check if the timestamp includes hours
    if len(timestamp.split(':')) == 3:
        return datetime.strptime(timestamp, '%H:%M:%S.%f')
    else:
        # Add a dummy hour for timestamps without hours and convert to datetime
        return datetime.strptime('00:' + timestamp, '%H:%M:%S.%f')
def extract_tx_rx_state_data(line):
    timestamp = line.split("\t")[0]
    if "TX:" in line and "RX:" in line:
        tx_value, rx_value = map(int, line.split("TX: ")[1].split(", RX: "))
        return timestamp, tx_value, rx_value, None
    elif "state:" in line:
        state_value = int(line.split("state: ")[1])
        return timestamp, None, None, state_value
    return timestamp, None, None, None

timestamps = []
tx_rx_ratios = []
state_values = []

with open(file_path, 'r') as file:
    for line in file:
        timestamp, tx_value, rx_value, state_value = extract_tx_rx_state_data(line.strip())
        if timestamp is not None:
            # Convert timestamp to a datetime object
            timestamp = parse_timestamp(timestamp)
            timestamps.append(timestamp)
            if tx_value is not None and rx_value is not None:
                tx_rx_ratio = rx_value / tx_value if tx_value != 0 else 0
                tx_rx_ratios.append((timestamp, tx_rx_ratio))
            if state_value is not None:
                state_values.append((timestamp, state_value))

# Preparing data for plotting
tx_rx_times, tx_rx_data = zip(*tx_rx_ratios) if tx_rx_ratios else ([], [])
state_times, state_data = zip(*state_values) if state_values else ([], [])

all_times = tx_rx_times + state_times
min_time, max_time = min(all_times), max(all_times)
#average of state_values
state_average = sum(state_data)/len(state_data)
print("Average of state values: ", state_average)  

# Plotting the data
fig, axs = plt.subplots(2, figsize=(10, 8), sharex=True)

# Plot TX/RX Ratio
axs[0].plot(tx_rx_times, tx_rx_data, marker='o', color='blue')
axs[0].set_ylabel('RX/TX Ratio')
axs[0].set_title('RX/TX Ratio and State Over Time')
axs[0].set_xlim(min_time, max_time)  # Synchronize x-axis

# Plot State
axs[1].scatter(state_times, state_data, marker='o', color='red')
axs[1].set_xlabel('Time')
axs[1].set_ylabel('State')
axs[1].set_xlim(min_time, max_time)  # Synchronize x-axis

# Format the x-axis
plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.gca().xaxis.set_major_locator(mdates.MinuteLocator())
plt.gcf().autofmt_xdate()  # Rotate date labels
plt.grid(True)
plt.show()
