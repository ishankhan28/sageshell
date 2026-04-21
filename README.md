
# 🐚 MyShell — A Linux Shell Prototype in C

> *"I didn't just want to use a shell. I wanted to understand what happens inside one."*
> — Built from scratch by **Ishan Khan**

---

## 👋 What is MyShell?

MyShell is a **custom Linux shell** written in C using low-level system calls.

Most of us type commands like `ls`, `cd`, or `grep` every day — but have you ever wondered what actually happens under the hood? This project answers exactly that.

Instead of relying on Bash or Zsh, MyShell implements the core loop of a shell from scratch — reading your input, understanding what you mean, and making the OS do the work. Along the way it adds some quality-of-life features that even standard shells don't always offer out of the box.

---

## ✨ Features

### 🔧 Core Shell Features
| Feature | Description |
|---|---|
| **Command Execution** | Run any standard Linux command — `ls`, `pwd`, `cat`, and more |
| **Process Handling** | Every command spawns a child process via `fork()` + `execvp()` |
| **Piping** | Chain commands together using `\|` — e.g. `ls \| grep .c` |
| **Input/Output** | Reads from stdin, writes to stdout, errors to stderr |

---

### 🧠 Smart Features (What makes MyShell special)

#### 💡 Auto-Suggestion
Tired of typing full command names? MyShell suggests completions as you type.

```
myshell> cl
  → clear  ← suggested
  → clang
```
Start typing and MyShell looks up matching commands — saving you keystrokes and time.

---

#### ✏️ Auto-Correction
Mistyped a command? MyShell catches it and suggests what you probably meant.

```
myshell> mkadir newfolder
  ⚠ Did you mean: mkdir ?  [y/n]
```
Uses string similarity to detect typos — no more "command not found" frustration.

---

#### 📊 CPU Utilization Monitor
Want to know how hard your system is working? Just ask:

```
myshell> cpu
  CPU Usage: 34.2%
  Cores    : 4
  Load Avg : 0.91, 1.02, 0.88
```
MyShell reads directly from `/proc/stat` to give you real-time CPU stats — no `top`, no `htop` needed.

---

#### 🛡️ Safer Remove Command (`srm`)
`rm -rf` is powerful — and dangerous. One wrong command and your files are gone forever.

MyShell introduces `srm` — a safer alternative:

```
myshell> srm important_file.txt
  ⚠  Are you sure you want to delete 'important_file.txt'? [y/n]: y
  ✅ Moved to trash. Restore with: restore important_file.txt
```

- Asks for confirmation before deleting
- Moves files to a `.trash` folder instead of permanent deletion
- Allows restore if you change your mind

---

## 🏗️ How It Works — The Shell Loop

Every shell, including Bash, runs the same fundamental loop. Here's MyShell's:

```
┌─────────────────────────────────────┐
│                                     │
│   1. Display prompt  myshell>       │
│              ↓                      │
│   2. Read user input                │
│              ↓                      │
│   3. Parse command + arguments      │
│              ↓                      │
│   4. fork() — create child process  │
│              ↓                      │
│   5. Child calls execvp()           │
│      (becomes the command)          │
│              ↓                      │
│   6. Parent calls wait()            │
│      (waits for child to finish)    │
│              ↓                      │
│   7. Back to Step 1 ↺               │
│                                     │
└─────────────────────────────────────┘
```

---

## ⚙️ System Calls Used

These are the Linux kernel system calls that power MyShell:

| System Call | Purpose | Where Used |
|---|---|---|
| `fork()` | Creates a child process | Every command execution |
| `execvp()` | Replaces child with actual command | After fork in child |
| `wait()` | Parent waits for child to finish | After fork in parent |
| `pipe()` | Creates read/write channel between processes | Piped commands |
| `open()` | Opens a file, returns file descriptor | File operations |
| `read()` | Reads from file descriptor | Input handling |
| `write()` | Writes to file descriptor | Output handling |
| `close()` | Closes file descriptor | Cleanup |

---

## 🗂️ Project Structure

```
myshell/
│
├── shell.c          # Main shell loop — reads input, dispatches commands
├── executor.c       # fork() + execvp() logic
├── parser.c         # Tokenizes input into command + args
├── pipe.c           # Handles piped commands
├── suggest.c        # Auto-suggestion engine
├── correct.c        # Auto-correction using string similarity
├── cpu.c            # CPU utilization reader from /proc/stat
├── srm.c            # Safer remove with trash + restore
└── README.md        # You are here 👋
```

---

## 🚀 Getting Started

### Prerequisites
- Linux (Ubuntu recommended)
- GCC compiler
- Make

### Build & Run

```bash
# Clone the repository
git clone https://github.com/yourusername/myshell.git
cd myshell

# Compile
gcc -o myshell shell.c executor.c parser.c pipe.c suggest.c correct.c cpu.c srm.c

# Run
./myshell
```

You'll see:
```
Welcome to MyShell 🐚
Type 'help' for available commands. Type 'exit' to quit.

myshell>
```

---

## 💻 Example Session

```bash
myshell> ls -l
total 48
-rwxr-xr-x 1 ishan ishan 16832 Mar 5 myshell
-rw-r--r-- 1 ishan ishan  2048 Mar 5 shell.c
...

myshell> ls | grep .c
shell.c
executor.c
parser.c

myshell> cpu
CPU Usage : 27.4%
Cores     : 4
Load Avg  : 0.45, 0.60, 0.72

myshell> srm shell.c
⚠  Are you sure you want to delete 'shell.c'? [y/n]: n
✅ Cancelled. File is safe.

myshell> mkadir test
⚠  Did you mean: mkdir ? [y/n]: y
✅ Running: mkdir test

myshell> exit
Goodbye! 👋
```

---

## 🧱 Key Concepts This Project Covers

If you're learning OS internals, this project touches:

- **Process creation** — how `fork()` duplicates a process
- **Process replacement** — how `exec()` loads a new program
- **Inter-process communication** — how `pipe()` connects processes
- **File descriptors** — 0 (stdin), 1 (stdout), 2 (stderr)
- **Zombie & orphan processes** — what happens when wait() is or isn't called
- **Reading /proc filesystem** — how Linux exposes kernel data to user space

---

## 🙋 About the Developer

**Ishan Khan**
B.Tech CSE — Integral University, Lucknow
📧 ishankhancs@gmail.com
🐙 [GitHub](https://github.com/yourusername)

> *This project was built from scratch as a learning exercise to deeply understand how operating systems manage processes, memory and I/O — not just to use a shell, but to think like one.*

---

## 📄 License
MIT License — feel free to learn from it, build on it, or break it. 😄
