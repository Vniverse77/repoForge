#include <iostream>
#include <string>
#include <vector>
#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <thread>
#include <chrono>
#include <csignal>
#include <array>
#include <memory>
#include <cstdio>

namespace fs = std::filesystem;

const std::string RESET = "\033[0m";
const std::string BOLD = "\033[1m";
const std::string WHITE = "\033[97m";
const std::string GREEN = "\033[32m";
const std::string BLUE = "\033[34m";
const std::string YELLOW = "\033[33m";
const std::string RED = "\033[31m";
const std::string CYAN = "\033[36m";

void clearScreen() {
    std::system("clear");
}

void sleepMs(int ms) {
    std::this_thread::sleep_for(std::chrono::milliseconds(ms));
}

void waitForEnter() {
    std::cout << "\n" << CYAN << "Press [ENTER] to continue..." << RESET;
    std::string dummy;
    std::getline(std::cin, dummy);
}

void showExitMessage() {
    clearScreen();
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << GREEN << "  Thank you for using repoForge\n" << RESET;
    std::cout << BOLD << CYAN << "  Developer: Neuwj\n" << RESET;
    std::cout << BOLD << CYAN << "  Contact:   neuwj@bk.ru\n" << RESET;
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << YELLOW << "  Stay safe.\n\n" << RESET;
}

void handleSigint(int sig) {
    showExitMessage();
    std::exit(0);
}

std::string execCommand(const std::string& cmd) {
    std::array<char, 128> buffer;
    std::string result;
    auto pipeDeleter = [](FILE* f) { if (f) pclose(f); };
    std::unique_ptr<FILE, decltype(pipeDeleter)> pipe(popen(cmd.c_str(), "r"), pipeDeleter);
    if (!pipe) return "";
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    if (!result.empty() && result.back() == '\n') result.pop_back();
    return result;
}

bool isDirEmpty(const std::string& path) {
    try {
        if (!fs::exists(path)) return true;
        return fs::directory_iterator(path) == fs::directory_iterator();
    } catch (...) {
        return true;
    }
}

std::string cleanPath(std::string path) {
    if (path.empty()) return path;
    size_t first = path.find_first_not_of(" \t\'\"");
    if (first == std::string::npos) return "";
    path.erase(0, first);
    path.erase(path.find_last_not_of(" \t\'\"") + 1);

    if (path.length() > 0 && path[0] == '~') {
        const char* home = std::getenv("HOME");
        if (home) path.replace(0, 1, std::string(home));
    }
    while (path.length() > 1 && path.back() == '/') {
        path.pop_back();
    }
    return path;
}

std::string escapeShellArg(const std::string& arg) {
    std::string escaped = "'";
    for (char c : arg) {
        if (c == '\'') escaped += "'\\''";
        else escaped += c;
    }
    escaped += "'";
    return escaped;
}

bool isValidPkgName(const std::string& name) {
    if (name.empty()) return false;
    for (char c : name) {
        if (!std::isalnum(c) && c != '-' && c != '_' && c != '+' && c != '.') return false;
    }
    return true;
}

void printBanner() {
    clearScreen();
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << BOLD << YELLOW << "   .===.   " << RESET << BOLD << "repoForge\n";
    std::cout << BOLD << YELLOW << "   | " << CYAN << "*" << YELLOW << " |   " << RESET << "Automated Repository Indexer | v0.0.9\n";
    std::cout << BOLD << YELLOW << "    \\ /    " << CYAN << "Made by Neuwj - neuwj@bk.ru\n";
    std::cout << BOLD << YELLOW << "     V     " << RESET << "\n";
    std::cout << BOLD << BLUE << "=================================================\n" << RESET;
    std::cout << CYAN << " System: APT, DNF, Pacman (Arch) & AUR (Docker) Ready!\n\n" << RESET;
}

void printError(const std::string& message) {
    std::cout << RED << "[-] ERROR: " << RESET << message << "\n";
}

void printStep(const std::string& message) {
    std::cout << BLUE << "[*] " << RESET << message << "...\n";
    sleepMs(400);
}

void printSuccess(const std::string& message) {
    std::cout << GREEN << "[+] " << RESET << BOLD << message << RESET << "\n";
}

std::string askInput(const std::string& question, bool isRequired = true, bool isPath = false) {
    std::string input;
    while (true) {
        std::cout << YELLOW << question << ": " << RESET;
        std::getline(std::cin, input);
        if (isPath) input = cleanPath(input);
        else {
            size_t first = input.find_first_not_of(" \t");
            if (first != std::string::npos) {
                input.erase(0, first);
                input.erase(input.find_last_not_of(" \t") + 1);
            } else input = "";
        }
        if (input.empty() && isRequired) {
            printError("This field cannot be empty.");
            continue;
        }
        return input;
    }
}


void handleAUR() {
    clearScreen();
    std::cout << BOLD << CYAN << "============================================================\n" << RESET;
    std::cout << BOLD << CYAN << "                 AUR AUTOMATION ENGINE                \n" << RESET;
    std::cout << BOLD << CYAN << "============================================================\n\n" << RESET;

    if (std::system("docker info > /dev/null 2>&1") != 0) {
        printError("Docker is not running! Please use 'sudo systemctl start docker' first.");
        waitForEnter();
        return;
    }

    const char* home = std::getenv("HOME");
    std::string sshDir = std::string(home) + "/.ssh";
    std::string pubKeyPath = "";

    printStep("Scanning for SSH Keys");
    if (fs::exists(sshDir + "/id_ed25519.pub")) pubKeyPath = sshDir + "/id_ed25519.pub";
    else if (fs::exists(sshDir + "/id_rsa.pub")) pubKeyPath = sshDir + "/id_rsa.pub";

    if (!pubKeyPath.empty()) {
        std::cout << GREEN << "[+] Found key: " << pubKeyPath << RESET << "\n";
        std::string useKey = askInput("Would you like to use this key? (Y/n)", false);
        if (useKey == "n" || useKey == "N") pubKeyPath = "";
    }

    if (pubKeyPath.empty()) {
        std::cout << YELLOW << "\nI couldn't find a suitable SSH key for AUR on your system.\n" << RESET;
        std::string createKey = askInput("Would you like me to generate one for you? (Y/n)", false);
        if (createKey != "n" && createKey != "N") {
            printStep("Generating new ed25519 SSH key");
            std::system("ssh-keygen -t ed25519 -C \"neuwj@aur.automation\" -f ~/.ssh/id_ed25519 -N \"\"");
            pubKeyPath = sshDir + "/id_ed25519.pub";
            printSuccess("SSH key generated successfully.");
        } else {
            printError("AUR operations cannot proceed without an SSH key.");
            waitForEnter();
            return;
        }
    }

    std::cout << "\n" << BOLD << BLUE << "--- AUR ACCOUNT NOTIFICATION ---\n" << RESET;
    std::string pubKeyContent = execCommand("cat " + pubKeyPath);
    std::cout << "Please copy the following public key and add it to your AUR My Account page:\n\n";
    std::cout << BOLD << pubKeyContent << RESET << "\n\n";
    askInput(CYAN + "Press [ENTER] to continue if you have added it" + RESET, false);

    printStep("Testing AUR connection");
    std::string sshTest = execCommand("ssh -o StrictHostKeyChecking=no -T aur@aur.archlinux.org 2>&1");
    if (sshTest.find("successfully authenticated") == std::string::npos) {
        std::cout << YELLOW << "[!] Warning: Connection could not be fully verified, but proceeding...\n" << RESET;
    }

    std::cout << "\n" << BOLD << BLUE << "--- PACKAGE METADATA ENTRY ---\n" << RESET;
    std::string pkgname = askInput("Package name (pkgname)");
    if (!isValidPkgName(pkgname)) {
        printError("Invalid package name! Only alphanumeric characters, '-', '_', '+', and '.' are allowed.");
        waitForEnter();
        return;
    }

    printStep("Cloning AUR repository for " + pkgname);
    std::string cloneCmd = "git clone ssh://aur@aur.archlinux.org/" + escapeShellArg(pkgname + ".git");
    if (std::system(cloneCmd.c_str()) != 0) {
        printError("Git clone failed! Check your internet connection or SSH key authorization.");
        waitForEnter();
        return;
    }

    std::string pkgver = askInput("Version (pkgver)");
    std::string pkgdesc = askInput("Short description (pkgdesc)");
    std::string url = askInput("Project homepage (url)");
    std::string depends = askInput("Dependencies (depends) [e.g., 'python' 'gcc']", false);
    std::string source = askInput("Source code link (.tar.gz/.zip etc.) (source)");

    std::cout << "\n" << BOLD << CYAN << "--- LICENSE SELECTION ---\n" << RESET;
    std::vector<std::string> licenses = {"GPL", "GPL3", "MIT", "Apache", "BSD", "ISC", "Unlicense", "Python", "Ruby", "Zlib"};
    for (size_t i = 0; i < licenses.size(); ++i) {
        std::cout << "[" << i + 1 << "] " << licenses[i] << "  ";
        if ((i + 1) % 5 == 0) std::cout << "\n";
    }
    std::string license = askInput("\nLicense Type (e.g., MIT)");

    printStep("Downloading source and calculating SHA-256");
    std::string tempFile = pkgname + "_temp";
    std::system(("wget -qO " + escapeShellArg(tempFile) + " " + escapeShellArg(source)).c_str());
    std::string sha256 = execCommand("sha256sum " + escapeShellArg(tempFile) + " | awk '{print $1}'");
    std::system(("rm -f " + escapeShellArg(tempFile)).c_str());
    if (sha256.empty()) sha256 = "SKIP";

    printStep("Generating PKGBUILD and .SRCINFO");
    std::ofstream pkgbuild(pkgname + "/PKGBUILD");
    pkgbuild << "pkgname=" << pkgname << "\npkgver=" << pkgver << "\npkgrel=1\npkgdesc=\"" << pkgdesc << "\"\n";
    pkgbuild << "arch=('any')\nurl=\"" << url << "\"\nlicense=('" << license << "')\n";
    if (!depends.empty()) pkgbuild << "depends=(" << depends << ")\nsource=(\"" << source << "\")\nsha256sums=('" << sha256 << "')\n";
    pkgbuild << "\npackage() {\n  install -Dm644 \"$srcdir\" \"$pkgdir/usr/share/doc/$pkgname/README\"\n}\n";
    pkgbuild.close();

    std::string dockerCmd = "docker run --rm -v " + escapeShellArg(fs::current_path().string() + "/" + pkgname) + ":/pkg archlinux bash -c "
    "\"pacman -Sy --noconfirm base-devel sudo git && useradd -m builduser && chown -R builduser /pkg && "
    "sudo -u builduser bash -c 'cd /pkg && makepkg --printsrcinfo > .SRCINFO'\"";

    if (std::system(dockerCmd.c_str()) == 0) printSuccess(".SRCINFO generated successfully!");
    else printError("Docker process failed!");

    if (askInput("Push to AUR? (Y/n)", false) != "n") {
        std::system(("cd " + pkgname + " && git add PKGBUILD .SRCINFO && git commit -m 'Automated push by Repo Forge' && git push origin master").c_str());
        printSuccess("Package is live!");
    }
    waitForEnter();
}



void initRepo() {
    std::cout << "\n" << BOLD << CYAN << "--- BUILDING REPOSITORY SKELETON ---" << RESET << "\n";
    std::string repoDir = askInput("Repository Root Directory (e.g., ~/myrepo)", true, true);
    try {
        fs::create_directories(repoDir + "/debian/pool/main");
        fs::create_directories(repoDir + "/rpm");
        fs::create_directories(repoDir + "/keys");
        std::ofstream gitignore(repoDir + "/.gitignore");
        gitignore << "# Security\n*.sec\n*.sk\n\n# Temp\n*.log\n.repodata.old/\n";
        printSuccess("Skeleton ready at: " + repoDir);
    } catch (const std::exception& e) { printError(e.what()); }
    waitForEnter();
}

void indexRepo() {
    std::cout << "\n" << BOLD << CYAN << "--- SCANNING & INDEXING ENGINE ---" << RESET << "\n";
    std::string repoDir = askInput("Repository Root Directory", true, true);
    if (!fs::exists(repoDir)) { printError("Directory not found!"); waitForEnter(); return; }

    std::string debianPool = repoDir + "/debian/pool/main";
    if (fs::exists(debianPool) && !isDirEmpty(debianPool)) {
        printStep("Indexing Debian packages");
        std::string codename = askInput("Codename (leave empty for 'stable')", false); if(codename.empty()) codename = "stable";
        std::string arch = askInput("Architecture (leave empty for 'amd64')", false); if(arch.empty()) arch = "amd64";
        std::string distsDir = repoDir + "/debian/dists/" + codename + "/main/binary-" + arch;
        fs::create_directories(distsDir);

        std::string cmd = "cd " + escapeShellArg(repoDir + "/debian") + " && dpkg-scanpackages pool/main /dev/null > " + escapeShellArg("dists/" + codename + "/main/binary-" + arch + "/Packages");
        if (std::system(cmd.c_str()) == 0) {
            std::system(("gzip -k -f " + escapeShellArg(distsDir + "/Packages")).c_str());
            printSuccess("Debian index updated.");
        }
    }
    waitForEnter();
}

void installTools() {
    std::cout << "\n" << BOLD << CYAN << "--- INSTALLING REQUIRED TOOLS ---" << RESET << "\n";
    bool hasApt = (std::system("command -v apt > /dev/null 2>&1") == 0);
    bool hasDnf = (std::system("command -v dnf > /dev/null 2>&1") == 0);
    bool hasPacman = (std::system("command -v pacman > /dev/null 2>&1") == 0);

    if (hasApt) {
        printStep("Debian/Ubuntu/MX Linux detected");
        std::system("sudo apt update && sudo apt install -y dpkg-dev apt-utils createrepo-c docker.io git openssh-client");
    } else if (hasDnf) {
        printStep("Fedora/RHEL detected");
        std::system("sudo dnf install -y createrepo_c dpkg docker git openssh-clients");
    } else if (hasPacman) {
        printStep("Arch Linux detected");
        std::system("sudo pacman -Sy --noconfirm dpkg createrepo_c docker git openssh");
    }
    printSuccess("Attempted installation. Don't forget to start Docker if needed.");
    waitForEnter();
}

void showGuide() {
    clearScreen();
    std::cout << BOLD << CYAN << "--- USER GUIDE ---\n" << RESET;
    std::cout << "1. Init: Creates repository subdirectories.\n2. Index: Scans and indexes packages.\n3. AUR: Prepares and pushes an Arch package via Docker.\n4. Install: Installs required tools.\n";
    askInput("Press [ENTER] to return", false);
}

int main() {
    std::signal(SIGINT, handleSigint);
    while(true) {
        printBanner();
        std::cout << "  [1] Create Repo Skeleton\n  [2] Scan and Index Repository\n  [3] AUR Automation Engine\n  [4] Install Required Tools\n  [0] User Guide\n  [q] Exit\n\n";
        std::string choice = askInput("Your choice", true, false);
        if (choice == "1") initRepo();
        else if (choice == "2") indexRepo();
        else if (choice == "3") handleAUR();
        else if (choice == "4") installTools();
        else if (choice == "0") showGuide();
        else if (choice == "q" || choice == "Q") { showExitMessage(); break; }
    }
    return 0;
}
