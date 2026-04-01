<div align="center">

# 🛠️ repoForge

**Automated Linux Repository Indexer & AUR Engine**

![Version](https://img.shields.io/badge/Version-0.0.9-orange?style=for-the-badge)
![License](https://img.shields.io/badge/License-GPL_v3-blue?style=for-the-badge&logo=gnu&logoColor=white)
![C++](https://img.shields.io/badge/C%2B%2B-00599C?style=for-the-badge&logo=c%2B%2B&logoColor=white)
![Linux](https://img.shields.io/badge/Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)
![Docker](https://img.shields.io/badge/Docker-2496ED?style=for-the-badge&logo=docker&logoColor=white)
![Arch Linux](https://img.shields.io/badge/Arch_Linux-1793D1?style=for-the-badge&logo=arch-linux&logoColor=white)
![AUR](https://img.shields.io/badge/AUR-1793D1?style=for-the-badge&logo=arch-linux&logoColor=white)
![Built For Linux](https://img.shields.io/badge/Built_for-Linux-FCC624?style=for-the-badge&logo=linux&logoColor=black)

Created by [Neuwj](https://github.com/Neuwj-00)

</div>

---

## 📋 Overview

**repoForge** is a high-performance terminal utility engineered in C++ to streamline the management of Linux package repositories and automate the Arch User Repository (AUR) workflow. It enables developers to generate complex repository architectures and prepare packages for deployment within seconds.

---

## ✨ Key Features

* **Repository Skeleton** — Automatically constructs the essential directory hierarchy (`pool`, `dists`, `keys`) for both APT and RPM repositories.
* **Scanning & Indexing** — Efficiently scans Debian packages to generate and update `Packages.gz` index files.
* **AUR Automation** — Utilizes Docker containers to generate PKGBUILD and `.SRCINFO` files, facilitating secure pushes to AUR via SSH.
* **Multi-Distro Support** — Built with native compatibility for APT (Debian/Ubuntu/MX Linux), DNF (Fedora/RHEL), and Pacman (Arch) ecosystems.
* **Smart Environment** — Proactively identifies and installs missing dependencies such as `dpkg-dev`, `createrepo_c`, and `docker` based on the host system.

---

## 🚀 Installation

### Debian / Ubuntu / MX Linux

```bash
# Add the GPG key for secure package verification
curl -fsSL [https://neuwj-00.github.io/byte-knight/keys/public.key](https://neuwj-00.github.io/byte-knight/keys/public.key) | sudo gpg --dearmor -o /usr/share/keyrings/byte-knight-keyring.gpg

# Add the official repository to your APT sources
echo "deb [signed-by=/usr/share/keyrings/byte-knight-keyring.gpg] [https://neuwj-00.github.io/byte-knight/debian](https://neuwj-00.github.io/byte-knight/debian) stable main" | sudo tee /etc/apt/sources.list.d/byte-knight.list

# Update package lists and install repoForge
sudo apt update
sudo apt install repoforge
```
### FEDORA / RHEL
```bash
# Add the repository configuration
sudo tee /etc/yum.repos.d/byte-knight.repo <<EOF
[byte-knight]
name=byte-knight Official Repository
baseurl=[https://neuwj-00.github.io/byte-knight/rpm/](https://neuwj-00.github.io/byte-knight/rpm/)
enabled=1
gpgcheck=0
EOF

# Install the package
sudo dnf install repoforge
```
### Build from source
```bash
# Clone the repository
git clone [https://github.com/Neuwj-00/byte-knight.git](https://github.com/Neuwj-00/byte-knight.git)
cd byte-knight/source

# Compile with C++17 standard
g++ -std=c++17 -O2 -o repoforge repoForge.cpp

# Install the binary to the system path
sudo install -Dm755 repoforge /usr/bin/repoforge

```
🛠️ Usage
After launching the utility, navigate the interactive menu to manage your repository:

Create Repo Skeleton: Initializes the repository root and subdirectory structure.

Scan and Index: Processes local .deb packages and refreshes indexing metadata.

AUR Automation Engine: Triggers the Docker-assisted secure packaging and AUR submission process.

Install Required Tools: Executes a system-specific installation of necessary backend utilities.

```bash
repoForge
├── .directory
├── README.md
├── debian
│   └── repoforge_0.0.9_amd64.deb
├── rpm 
└── source
    └── repoForge.cpp

```
📫 Contact
Developer: Neuwj

Email: neuwj@bk.ru

GitHub: Neuwj-00
