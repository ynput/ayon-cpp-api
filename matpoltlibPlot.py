import matplotlib.pyplot as plt

def plot_graph(data):
    # Extract keys and values from the dictionary
    keys = list(data.keys())
    values = list(data.values())

    # Plot the graph
    plt.plot(keys, values, marker='o', linestyle='-')

    # Add labels and title
    plt.xlabel('X-axis Label')
    plt.ylabel('Y-axis Label')
    plt.title('Simple Graph from Dictionary')

    # Show the graph
    plt.show()

# Example dictionary
data_dict = {1: 10, 2: 20, 3: 15, 4: 25, 5: 30}

# Plot the graph using the example dictionary
plot_graph(data_dict)

