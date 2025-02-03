# Pyro Shell (Pyronix)

## 🔥 The Ultimate Minimalist Shell for Cryonix/Linux
![pyronix](https://github.com/user-attachments/assets/4eecbc99-b6b2-4ec6-883e-5addc55acae4)

Pyro Shell (Pyronix) is a blazing-fast, lightweight, and feature-rich shell designed for the Cryonix/Linux userland. Inspired by traditional UNIX shells like `sh` and `zsh`, Pyronix combines efficiency with modern enhancements to create a seamless command-line experience.

---

## 🚀 Features

✅ **Run Executables** – Automatically scans `/usr/bin/` and allows you to execute programs effortlessly.  
✅ **Command Piping (`|`)** – Chain commands together for powerful workflows.  
✅ **Logical Operators (`&&`, `||`)** – Execute commands conditionally.  
✅ **Environment Variable Support** – Handles `$VAR` expansion just like traditional shells.  
✅ **Built-in Commands (`cd`, `clear`, etc.)** – Fully functional navigation and control.  
✅ **Command History** – Navigate with the up/down arrow keys.  
✅ **Tab Completion** – Auto-complete commands and directories.  
✅ **Process Management** – Runs processes in the foreground and background.  
✅ **Cross-Compatible** – Designed to work on any Linux distribution.


---

## 🔧 Installation

To install Pyro Shell on your system, follow these steps:

```sh
# Clone the repository
git clone https://github.com/yourusername/pyrosh.git

# Navigate to the directory
cd pyrosh

# Compile the source code
make

# Run Pyrosh
./pyrosh
```

For system-wide installation:
```sh
sudo make install
```

---

## 🛠 Usage

Once inside Pyrosh, you can execute commands just like any other shell:

```sh
# Running a command
echo "Hello, Pyronix!"

# Changing directories
cd /path/to/directory

# Listing files
ls -lah

# Using pipes
echo "hello" | grep h

# Logical operators
mkdir test_dir && cd test_dir || echo "Failed to create directory!"

# Running background tasks
./long_running_process &
```

---

## 🏗 Built-in Commands

| Command | Description |
|---------|-------------|
| `cd <dir>` | Change directory |
| `clear` | Clear the terminal |
| `exit` | Exit the shell |
| `set` | Set an environment variable |
| `unset` | Unset an environment variable |
| `history` | View command history |

---

## 🏎 Performance
Pyronix is designed for speed and minimal resource consumption, making it perfect for embedded systems, IoT devices, and lightweight Linux distributions.

---

## 💡 Future Roadmap
- [ ] **Job Control** – Support for background and foreground process management.
- [ ] **Scripting Support** – Execute `.sh` scripts natively.
- [ ] **Advanced Auto-Completion** – Smarter suggestions based on command context.
- [ ] **Plugin System** – Extend functionality with custom modules.

---

## 🤝 Contributing
We welcome contributions! Feel free to submit issues, feature requests, or pull requests.

1. Fork the repository
2. Create a new branch (`git checkout -b feature-xyz`)
3. Commit your changes (`git commit -m "Added feature XYZ"`)
4. Push to your fork (`git push origin feature-xyz`)
5. Open a pull request

---

## 📜 License
Pyronix is open-source and licensed under the MIT License.


