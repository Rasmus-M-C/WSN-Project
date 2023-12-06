import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta

file_path = 'loglistener18.txt'  # Replace with the path to your text file
def extract_tx_rx_state_power_data(line):
    timestamp = line.split("\t")[0]
    if "TX:" in line and "RX:" in line:
        tx_value, rx_value = map(int, line.split("TX: ")[1].split(", RX: "))
        return timestamp, tx_value, rx_value, None, None
    elif "state:" in line:
        state_value = int(line.split("state: ")[1])
        return timestamp, None, None, state_value, None
    elif "Total power usage =" in line:
        power_value = float(line.split("Total power usage =")[1].split("mAh")[0].strip())
        return timestamp, None, None, None, power_value
    return timestamp, None, None, None, None
def parse_power_data(power_values):
    corrected_values = []
    previous_value = 0
    for timestamp, value in power_values:
        if value < previous_value:
            value += 65522.8896  # Adjust for reset
        corrected_values.append((timestamp, value))
        previous_value = value
    return corrected_values
def parse_timestamp(timestamp):
    # Check if the timestamp includes hours
    if len(timestamp.split(':')) == 3:
        return datetime.strptime(timestamp, '%H:%M:%S.%f')
    else:
        # Add a dummy hour for timestamps without hours and convert to datetime
        return datetime.strptime('00:' + timestamp, '%H:%M:%S.%f')

timestamps = []
tx_rx_ratios = []
state_values = []
power_values = []

with open(file_path, 'r') as file:
    for line in file:
        timestamp, tx_value, rx_value, state_value, power_value = extract_tx_rx_state_power_data(line.strip())
        if timestamp is not None:
            timestamp = parse_timestamp(timestamp)
            timestamps.append(timestamp)
            if tx_value is not None and rx_value is not None:
                tx_rx_ratio = rx_value / tx_value if rx_value != 0 else 0
                tx_rx_ratios.append((timestamp, tx_rx_ratio))
            if state_value is not None:
                state_values.append((timestamp, state_value))
            if power_value is not None:
                power_values.append((timestamp, power_value))
power_values = parse_power_data(power_values)
# Preparing data for plotting
tx_rx_times, tx_rx_data = zip(*tx_rx_ratios) if tx_rx_ratios else ([], [])
state_times, state_data = zip(*state_values) if state_values else ([], [])
power_times, power_data = zip(*power_values) if power_values else ([], [])


# Plotting the data
fig, axs = plt.subplots(3, figsize=(10, 12), sharex=True)

# Plot TX/RX Ratio
axs[0].plot(tx_rx_times, tx_rx_data, marker='o', color='blue')
axs[0].set_ylabel('RX/TX Ratio')
axs[0].set_title('RX/RX Ratio, State, and Power Consumption Over Time')

# Plot State
axs[1].scatter(state_times, state_data, marker='o', color='red')
axs[1].set_ylabel('State')

# Plot Power Consumption
axs[2].plot(power_times, power_data, marker='o', color='green')
axs[2].set_xlabel('Time')
axs[2].set_ylabel('Power Consumption (mAs)')

# Format the x-axis
plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.gca().xaxis.set_major_locator(mdates.MinuteLocator())
plt.gcf().autofmt_xdate()  # Rotate date labels
plt.grid(True)
plt.show()
