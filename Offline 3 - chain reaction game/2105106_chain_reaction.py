import tkinter as tk
from tkinter import messagebox

ROWS, COLS = 9, 6
CELL_SIZE = 60
DELAY = 1000  # milliseconds between chain reactions

# Game state
BOARD = [['0' for _ in range(COLS)] for _ in range(ROWS)]
PLAYER = 'R'  # R = Red, B = Blue
explosion_queue = []  # for animation

def critical_mass(i, j):
    if (i == 0 or i == ROWS - 1) and (j == 0 or j == COLS - 1):
        return 2
    elif i == 0 or i == ROWS - 1 or j == 0 or j == COLS - 1:
        return 3
    else:
        return 4

def draw_board():
    canvas.delete("all")
    canvas.create_line(0, 1, 540, 1, fill="black", width=2) # horizontal line at the top
    canvas.create_line(1, 0, 1, 540, fill="black", width=2) # vertical line on the left

    
    for i in range(ROWS):
        for j in range(COLS):
            x1, y1 = j * CELL_SIZE, i * CELL_SIZE
            x2, y2 = x1 + CELL_SIZE, y1 + CELL_SIZE

            # print(f"Drawing cell ({i}, {j}) at ({x1}, {y1}) to ({x2}, {y2})")
            canvas.create_rectangle(x1, y1, x2, y2, outline="black")
            val = BOARD[i][j]
            if val != '0':
                count, color = int(val[:-1]), val[-1]
                color_val = "red" if color == 'R' else "blue"
                draw_orbs(x1, y1, min(count, 3), color_val)  # draw up to 3 orbs visually



def draw_orbs(x, y, count, color):
    r = 8  # radius of each orb
    positions = {
        1: [(x + CELL_SIZE // 2, y + CELL_SIZE // 2)],
        2: [(x + CELL_SIZE // 3, y + CELL_SIZE // 2), (x + 2 * CELL_SIZE // 3, y + CELL_SIZE // 2)],
        3: [(x + CELL_SIZE // 2, y + CELL_SIZE // 3),
            (x + CELL_SIZE // 3, y + 2 * CELL_SIZE // 3),
            (x + 2 * CELL_SIZE // 3, y + 2 * CELL_SIZE // 3)]
    }
    for cx, cy in positions.get(count, []):
        canvas.create_oval(cx - r, cy - r, cx + r, cy + r, fill=color, outline=color)


def cell_clicked(event):
    global PLAYER
    if explosion_queue:
        return  # prevent clicking while animation is happening
    j, i = event.x // CELL_SIZE, event.y // CELL_SIZE
    val = BOARD[i][j]
    if val == '0' or val.endswith(PLAYER):
        add_orb(i, j, PLAYER)
        process_explosions()
        draw_board()

def add_orb(i, j, color):
    val = BOARD[i][j]
    if val == '0':
        BOARD[i][j] = f"1{color}"
    else:
        count = int(val[:-1]) + 1
        BOARD[i][j] = f"{count}{color}"
    if int(BOARD[i][j][:-1]) >= critical_mass(i, j):
        explosion_queue.append((i, j, color))

def process_explosions():
    global explosion_queue
    if not explosion_queue:
        finish_turn()
        return

    current_wave = explosion_queue
    explosion_queue = []

    for i, j, color in current_wave:
        count = int(BOARD[i][j][:-1])
        new_count = count - critical_mass(i, j)
        if new_count <= 0:
            BOARD[i][j] = '0'
        else:
            BOARD[i][j] = f"{new_count}{color}"

        
        for dx, dy in [(-1, 0), (1, 0), (0, -1), (0, 1)]:
            ni, nj = i + dx, j + dy
            if 0 <= ni < ROWS and 0 <= nj < COLS:
                val = BOARD[ni][nj]
                if val == '0':
                    BOARD[ni][nj] = f"1{color}"
                else:
                    count = int(val[:-1]) + 1
                    BOARD[ni][nj] = f"{count}{color}"

                # Only schedule if not already in explosion_queue
                if int(BOARD[ni][nj][:-1]) >= critical_mass(ni, nj):
                    if (ni, nj, color) not in explosion_queue:
                        explosion_queue.append((ni, nj, color))

    draw_board()
    root.after(DELAY, process_explosions)



def check_winner():
    red, blue = 0, 0
    for row in BOARD:
        for cell in row:
            if cell.endswith('R'):
                red += 1
            elif cell.endswith('B'):
                blue += 1

    total = red + blue
    if total <= 1:
        return None  # too early
    if red == 0:
        return 'B'
    elif blue == 0:
        return 'R'
    return None

def reset_board():
    global BOARD, PLAYER, explosion_queue
    BOARD = [['0' for _ in range(COLS)] for _ in range(ROWS)]
    PLAYER = 'R'
    explosion_queue = []
    draw_board()
    update_status_label()



def start_human_vs_human():
    print("Starting Human vs Human mode")
    reset_board()

def start_human_vs_ai():
    print("Starting Human vs AI mode (placeholder)")
    reset_board()
   

def start_ai_vs_ai():
    print("Starting AI vs AI mode")
    reset_board()
    # run_ai_vs_ai_turn()

    


def on_mode_change(*args):
    mode = GAME_MODE.get()
    if mode == "Human vs Human":
        start_human_vs_human()
    elif mode == "Human vs AI":
        start_human_vs_ai()
    elif mode == "AI vs AI":
        start_ai_vs_ai()


def write_gamestate_to_file(player):
    with open("gamestate.txt", "w") as f:
        header = "Human Move:" if player == "R" else "AI Move:"
        f.write(f"{header}\n")
        for row in BOARD:
            row_str = " ".join(cell for cell in row)
            f.write(f"{row_str}\n")

import time

def finish_turn():
    global PLAYER
    winner = check_winner()
    if winner:
        messagebox.showinfo("Game Over", f"{'Red' if winner == 'R' else 'Blue'} wins!")
        reset_board()
    else:
        if GAME_MODE.get() == "Human vs AI" and PLAYER == "R":
            draw_board()  # Ensure human move is drawn
            write_gamestate_to_file(PLAYER)
            PLAYER = 'B'
            update_status_label()
            root.after(100, wait_for_ai_and_apply_move)  # slight delay to allow UI update
        elif GAME_MODE.get() == "Human vs AI" and PLAYER == "B":
            # print("ekhane asche ")
            write_gamestate_to_file(PLAYER)
            PLAYER = 'R'
            update_status_label()
        else:
            PLAYER = 'B' if PLAYER == 'R' else 'R'
            update_status_label()


def wait_for_ai_and_apply_move():
    print("Waiting for AI move...")
    while True:
        try:
            with open("gamestate.txt", "r") as f:
                lines = f.readlines()
                if lines[0].strip() == "AI Move:":
                    print("AI move detected.")

                    new_board = [line.strip().split() for line in lines[1:]]
                    move_found = False

                    for i in range(ROWS):
                        for j in range(COLS):
                            if BOARD[i][j] != new_board[i][j]:
                                ai_move_i, ai_move_j = i, j
                                move_found = True
                                break
                        if move_found:
                            break

                    if not move_found:
                        print("No move detected in board difference!")
                        return

                    # Apply only the orb, not the full board
                    add_orb(ai_move_i, ai_move_j, 'B')
                    # Let explosion animation handle rest
                    process_explosions()
                    
                    draw_board()
                    
                    return  # Let turn finish naturally after explosions
        except Exception as e:
            print("Error reading file:", e)

        time.sleep(0.5)




def update_status_label():
    if PLAYER == 'R':
        status_label.config(text="Red's Turn (Human)", fg="red")
    else:
        if GAME_MODE.get() == "Human vs AI":
            status_label.config(text="Blue's Turn (AI)", fg="blue")
        else:
            status_label.config(text="Blue's Turn (Human)", fg="blue")



# Tkinter setup
root = tk.Tk()
# Game mode state

GAME_MODE = tk.StringVar(value="Human vs Human")
GAME_MODE.trace("w", on_mode_change)


# Frame for left-side controls
control_frame = tk.Frame(root)
control_frame.pack(side=tk.LEFT, fill=tk.Y, padx=10, pady=10)

# Dropdown for game mode selection
tk.Label(control_frame, text="Game Mode:", font=("Arial", 12)).pack(anchor="w")
mode_menu = tk.OptionMenu(control_frame, GAME_MODE, "Human vs Human", "Human vs AI", "AI vs AI")
mode_menu.config(width=15, font=("Arial", 12))
mode_menu.pack(anchor="w", pady=5)

# Add this just after the mode_menu in control_frame
status_label = tk.Label(control_frame, text="Red's Turn", font=("Arial", 14), fg="red")
status_label.pack(anchor="w", pady=10)




root.title("Chain Reaction - 2 Player (Animated)")
canvas = tk.Canvas(root, width=COLS * CELL_SIZE, height=ROWS * CELL_SIZE)
canvas.pack(side=tk.RIGHT, padx=10, pady=10)

canvas.bind("<Button-1>", cell_clicked)

draw_board()
root.mainloop()