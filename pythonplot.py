import matplotlib.pyplot as plt
import matplotlib.dates as mdates
from datetime import datetime, timedelta

file_path = 'loglistener27.txt'  # Replace with the path to your text file
def extract_tx_rx_state_power_data(line):
    timestamp = line.split("\t")[0]
    if "TX:" in line and "RX:" in line:
        tx_value, rx_value = map(int, line.split("TX: ")[1].split(", RX: "))
        return timestamp, tx_value, rx_value, None, None, None, None, None, None
    elif "state:" in line:
        state_value = int(line.split("state: ")[1])
        return timestamp, None, None, state_value, None, None, None, None, None
    elif "Total power usage A" in line:
        power_value_A = float(line.split("Total power usage A =")[1].split("mAh")[0].strip())
        return timestamp, None, None, None, power_value_A, None, None, None, None
    elif "Total power usage B" in line:
        power_value_B = float(line.split("Total power usage B =")[1].split("mAh")[0].strip())
        return timestamp, None, None, None, None, power_value_B, None, None, None
    elif "Total power usage C" in line:
        power_value_C = float(line.split("Total power usage C =")[1].split("mAh")[0].strip())
        return timestamp, None, None, None, None, None, power_value_C, None, None
    elif "Sending B" in line: 
        return timestamp, None, None, None, None, None, None, 1024*0.7, None
    elif "Sending C" in line: 
        return timestamp, None, None, None, None, None, None, None, 1024*0.5
    
    return timestamp, None, None, None, None, None, None, None, None
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
power_values_A = []
power_values_B = []
power_values_C = []
power_diff = []
sendingB = []
sendingC = []

with open(file_path, 'r') as file:
    for line in file:
        timestamp, tx_value, rx_value, state_value, power_value_A, power_value_B, power_value_C, sending_B, sending_C = extract_tx_rx_state_power_data(line.strip())
        if timestamp is not None:
            timestamp = parse_timestamp(timestamp)
            timestamps.append(timestamp)
            if tx_value is not None and rx_value is not None:
                tx_rx_ratio = (rx_value * 1024) / tx_value if rx_value != 0 else 0
                tx_rx_ratios.append((timestamp, min(tx_rx_ratio, 1024)))
            if state_value is not None:
                state_values.append((timestamp, state_value))
            if power_value_A is not None:
                power_values_A.append((timestamp, power_value_A))
                #power_diff.append((timestamp, power_value_A - power_values_A[-1][1]))
            if power_value_B is not None:
                if power_values_B != []:
                    power_diff.append((timestamp, power_value_B - power_values_B[-1][1]))
                power_values_B.append((timestamp, power_value_B))
            if power_value_C is not None:
                power_values_C.append((timestamp, power_value_C))
                #power_diff.append((timestamp, power_value_C - power_values_C[-2][1]))
            if sending_B is not None:
                sendingB.append((timestamp, sending_B))
            if sending_C is not None:
                sendingC.append((timestamp, sending_C))

                
power_values = parse_power_data(power_values_A)
# Preparing data for plotting
tx_rx_times, tx_rx_data = zip(*tx_rx_ratios) if tx_rx_ratios else ([], [])
state_times, state_data = zip(*state_values) if state_values else ([], [])
#power_times, power_data = zip(*power_values) if power_values else ([], [])
power_times, power_diff_data = zip(*power_diff) if power_diff else ([], [])
sendingB_times, sendingB_data = zip(*sendingB) if sendingB else ([], [])
sendingC_times, sendingC_data = zip(*sendingC) if sendingC else ([], [])


# Plotting the data
fig, axs = plt.subplots(3, figsize=(10, 12), sharex=True)

# Plot TX/RX Ratio
axs[0].plot(tx_rx_times, tx_rx_data, marker='o', color='blue')
axs[0].scatter(sendingB_times, sendingB_data, marker='o', color='green')
axs[0].scatter(sendingC_times, sendingC_data, marker='o', color='purple')
axs[0].set_ylabel('RX/TX Ratio')
axs[0].set_title('RX/RX Ratio, State, and Power Consumption Over Time')

# Plot State
axs[1].scatter(state_times, state_data, marker='o', color='red')
axs[1].set_ylabel('State')

# Plot Power Consumption
axs[2].plot(power_times, power_diff_data, color='green')
axs[2].set_xlabel('Time') 
axs[2].set_ylabel('Power Consumption (mAs)')

# Format the x-axis
plt.gca().xaxis.set_major_formatter(mdates.DateFormatter('%H:%M:%S'))
plt.gca().xaxis.set_major_locator(mdates.MinuteLocator())
plt.gcf().autofmt_xdate()  # Rotate date labels
plt.grid(True)
plt.show()
