'''
Visualize the output
from mask, and address, to visualize the output
Input format: bankmask, rankmask, column mask, row mask, bank address, rank address, column address, row address
output : 2D image

Default parameters:
16banks, 2rank, 1024 column, 16k row

Example: 0 0 0 7ff d 1 6b 8ec//0 0 0 7ff 3 1 98 1135f//f 0 3ff 1ffff 7 1 6b 10010//
It means that 
bank mask = 0, rank mask = 0, column mask = 0, row mask = 7ff
bank address = d, rank address = 1, column address = 6b, row address = 8ec
bank mask = 0, rank mask = 0, column mask = 0, row mask = 7ff
bank address = 3, rank address = 1, column address = 98, row address = 1135f
bank mask = f, rank mask = 0, column mask = 3ff, row mask = 1ffff
bank address = 7, rank address = 1, column address = 6b, row address = 10010

'''
import numpy as np
import matplotlib.pyplot as plt
import concurrent.futures
from matplotlib.backends.backend_pdf import PdfPages


# Default parameters
numofBanks = 16
NUM_RANKS = 2
NUM_COLUMNS = 1024
NUM_ROWS = 128 * 1024

COLUMN_GRANULARITY = 8
ROW_GRANULARITY = 1024

import numpy as np

def hex_to_bin(hex_val):
    # Convert the hexadecimal value to a binary string with leading zeros
    bin_val = bin(hex_val)[2:].zfill(32)
    return bin_val

from collections import deque

def flip_bit_combinations(binary_str):
    # Initialize the queue with the original binary string
    queue = deque([binary_str])

    # Initialize the set of visited strings with the original binary string
    visited = set([binary_str])

    # Loop until the queue is empty
    while queue:
        # Dequeue the next string from the queue
        current_str = queue.popleft()

        # Convert the string to a list of characters
        chars = list(current_str)

        # Loop over each bit in the binary string
        for i in range(len(chars)):
            # Flip the bit
            orig = chars[i]
            chars[i] = '0' if chars[i] == '1' else '0'

            # Convert the list of characters back to a string
            new_str = ''.join(chars)

            # If the new string has not been visited, add it to the queue and visited set
            if new_str not in visited:
                queue.append(new_str)
                visited.add(new_str)

            # Flip the bit back to its original value
            chars[i] = orig

    # Convert the visited set to a list and return it
    return list(visited)



def get_possible_values(hex_val):
    # Convert the hexadecimal value to binary
    bin_val = hex_to_bin(hex_val)

    # for all bits that are 1, there are two possible values (0 or 1)
    # for all bits that are 0, there is only one possible value (0)

    possible_combinations = flip_bit_combinations(bin_val)

    # Filter out the combinations that do not have the same position of 1's as the original binary string
    # if possible_combinations has 1, but bin_val has 0, then it is not possible
    # if possible_combinations has 0, but bin_val has 1, then it doesn't matter
    filtered_combinations = possible_combinations

    #convert binary to hex
    possible_values = [(int(''.join(comb), 2)) for comb in filtered_combinations]
    return possible_values

def get_affected_bits(bank_mask, rank_mask, column_mask, row_mask, bank_addr, rank_addr, column_addr, row_addr):
    # Compute the affected bits for each dimension
    if bank_mask > 1:
        bank_range = np.array(get_possible_values(bank_mask), dtype=np.uint64)| (bank_addr & ~bank_mask)
    else:
        bank_range = np.array(get_possible_values(bank_mask), dtype=np.uint64)|bank_addr
    if rank_mask > 1:
        rank_range = np.array(get_possible_values(rank_mask), dtype=np.uint64)| (rank_addr & ~rank_mask)
    else:
        rank_range = np.array(get_possible_values(rank_mask), dtype=np.uint64)|rank_addr
    if column_mask > 1:
        column_range = np.array(get_possible_values(column_mask), dtype=np.uint64)| (column_addr & ~column_mask)
    else:
        column_range = np.array(get_possible_values(column_mask), dtype=np.uint64)|column_addr
    if row_mask > 1:
        row_range = np.array(get_possible_values(row_mask), dtype=np.uint64)| (row_addr & ~row_mask)
    else:  
        row_range = np.array(get_possible_values(row_mask), dtype=np.uint64)|row_addr

    # sort the ranges 
    bank_range.sort()   
    rank_range.sort()
    column_range.sort()
    row_range.sort()

    # Convert the ranges to bit positions
    bank_pos = bank_range
    rank_pos = rank_range
    column_pos = column_range
    row_pos = row_range

    return bank_pos, rank_pos, column_pos, row_pos

def visualize_error_position(img, bank_mask, rank_mask, column_mask, row_mask, bank_addr, rank_addr, column_addr, row_addr):
    # Compute the affected bit positions
    bank_pos, rank_pos, column_pos, row_pos = get_affected_bits(bank_mask, rank_mask, column_mask, row_mask, bank_addr, rank_addr, column_addr, row_addr)

    # Create the images

    for bank in range(numofBanks):
        for rank in range(NUM_RANKS):
            if bank in bank_pos and rank in rank_pos:
                # Create an array of all white pixels
                # Set the pixels corresponding to affected rows and columns to black
                for row in np.unique(np.array(row_pos)//ROW_GRANULARITY):
                    for col in np.unique(np.array(column_pos)//COLUMN_GRANULARITY):
                        img[bank, rank, row, col] = 255
            

    
    

'''
Give multiple inputs, and visualize all. 
Input : 0 0 0 7ff b 0 ba 7b90//0 0 0 0 3 0 57 160ea//0 0 0 0 f 1 b2 199e5//0 0 0 0 7 1 b8 192e5//0 0 0 7ff b 1 24a 16c17//
output : 
visualize_error_position(pdf, 0, 0, 0, 7ff, b, 0, ba, 7b90)
visualize_error_position(pdf, 0, 0, 0, 0, 3, 0, 57, 160ea)
visualize_error_position(pdf, 0, 0, 0, 0, f, 1, b2, 199e5)
visualize_error_position(pdf, 0, 0, 0, 0, 7, 1, b8, 192e5)
visualize_error_position(pdf, 0, 0, 0, 7ff, b, 1, 24a, 16c17)
'''
def visualize_input(input_str):
    # Convert the input string to a list of lists
    input_lists = input_str.split('//')
    input_lists = [input_list.split(',')[:-1] for input_list in input_lists]

    # Initialize the image array
    img = np.zeros((numofBanks, NUM_RANKS, NUM_ROWS//ROW_GRANULARITY, NUM_COLUMNS//COLUMN_GRANULARITY), dtype=np.uint8)

    # Process each input list and update the image array
    for input_list in input_lists:
        for input_str in input_list:
            input_list = input_str.split(' ')
            visualize_error_position(img, int(input_list[0],16), int(input_list[1],16), int(input_list[2],16), int(input_list[3],16), int(input_list[4],16), int(input_list[5],16), int(input_list[6],16), int(input_list[7],16))

    # Return the image array
    return img

def visualize_all(pdf, input_str):
    # Split the input string and create a list of input strings
    input_lists = input_str.split('//')
    input_lists = input_lists[:-1]

    # Create a thread pool with the desired number of threads
    with concurrent.futures.ThreadPoolExecutor(max_workers=4) as executor:
        # Process each input string in parallel and save the resulting image to the PDF
        for i, img in enumerate(executor.map(visualize_input, input_lists)):
            # Create a new figure and plot the image
            fig, axs = plt.subplots(numofBanks, 2, figsize=(70, 500))
            fig.suptitle('Error Visualization', fontsize=70)
            for bank in range(numofBanks):
                for rank in range(NUM_RANKS): 
                    axs[bank, rank].imshow(img[bank,rank], cmap='gray_r', vmin=0, vmax=255)
                    axs[bank,rank].set_title('Bank {}, Rank {}'.format(bank, rank), fontsize=70)
                    # x axis label
                    axs[bank,rank].set_xlabel('Column', fontsize=70)
                    # y axis label
                    axs[bank,rank].set_ylabel('Row', fontsize=70)

            # Save the figure to the PDF and close it
            
            pdf.savefig(fig, bbox_inches='tight')
            plt.close()

import os 
if __name__ == '__main__':
    files = os.listdir('visual')
    for file in files:
        filename = file.split('.')[0]
        if not(filename == "not_clustered_single_bank" or filename == "not_clustered_multi_bank"):
            continue
        pdf = PdfPages('visual/'+filename+'.pdf')
        #read 2nd line from file
        #read 3rd line from file
        fd = open('visual/'+file, 'r')
        lines = fd.readlines()
        visualize_all(pdf, lines[1])
        pdf.close()