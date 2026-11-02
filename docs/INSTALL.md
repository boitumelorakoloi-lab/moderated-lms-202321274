# Installation Guide (for the school technician)

Moderated Learning Management System · release 1.1.0

This guide installs the LMS on one school computer, the **LMS PC**. Everybody uses the program
on that PC: either at its own keyboard, or from other lab and staff PCs over the school network
(SSH). The database never leaves the LMS PC, and nothing needs the internet (NFR-03). The
deployment diagram is in the final report (section 8) and in `docs/uml/9-deployment.drawio`.

| You need | Details |
|---|---|
| LMS PC | Any 64-bit PC from the last 10 years: 4 GB RAM, 20 GB free disk |
| Operating system | **Ubuntu 24.04 LTS** (recommended, up to 20 users at once) or Windows 10/11 (one user at a time) |
| Package | `moderated-lms_1.1.0_amd64.deb` (Ubuntu) or `moderated-lms-1.1.0-windows.zip` (Windows), from the GitHub release page |
| Time | About 30 minutes |
| Skills | Typing commands in a terminal, creating a user account |

Commands you type are shown in grey boxes. `<LMS-PC-IP>` means the LMS PC's address on the school
network (find it with `hostname -I`).

---

## Part A - Ubuntu LMS PC (recommended)

### A1. Install the program

1. Copy `moderated-lms_1.1.0_amd64.deb` to the LMS PC (USB stick is fine).
2. Open a terminal in that folder and run:

   ```bash
   sudo apt install ./moderated-lms_1.1.0_amd64.deb
   ```

   `apt` also installs the two libraries the program needs (SQLite and libsodium). If the PC has
   no internet at all, install them first from the Ubuntu install media: `libsqlite3-0` and
   `libsodium23`.
3. Check it worked: `lms --help` prints the usage text.

### A2. Create the data folder and the `lms` account

All data lives in one folder that only the program's own Linux account can read. This keeps
learners' records private (Data Protection Act, 2011) even from other users of the PC.

```bash
sudo adduser lms                      # choose a password - this is the "door" password for SSH
sudo mkdir -p /srv/lms/submissions /srv/lms/backups
sudo chown -R lms:lms /srv/lms
sudo chmod -R 700 /srv/lms
```

### A3. First start: create the administrator

```bash
sudo -u lms lms --db /srv/lms/lms.db --files /srv/lms/submissions
```

The database is empty, so the program asks for the administrator's username, full name and
password (at least 8 characters). Give these to the school's LMS administrator, then press
Enter at the empty `Username` prompt to quit.

### A4. Let people use it from other PCs (SSH)

1. Install the SSH server: `sudo apt install openssh-server`
2. Make the `lms` account open the program straight away and nothing else. Add these lines to the
   end of `/etc/ssh/sshd_config` (`sudo nano /etc/ssh/sshd_config`):

   ```
   Match User lms
       ForceCommand /usr/bin/lms --db /srv/lms/lms.db --files /srv/lms/submissions
       AllowTcpForwarding no
       X11Forwarding no
   ```

3. Restart SSH: `sudo systemctl restart ssh`
4. Only allow the school network (change the address range to the school's):

   ```bash
   sudo ufw allow from 192.168.1.0/24 to any port 22
   sudo ufw enable
   ```

5. On a lab or staff PC (Windows 10/11 has SSH built in), open **Command Prompt** and type:

   ```
   ssh lms@<LMS-PC-IP>
   ```

   Type the door password from step A2. The LMS login screen appears; each person then logs in
   with their **own** LMS username and password. To make it one click, put that command in a
   desktop shortcut.

Students who hand in work from a lab PC must first copy the file to the LMS PC (for example into
a shared `inbox` folder you create, or with `scp file.pdf lms@<LMS-PC-IP>:`), because the program
reads files from the LMS PC's disk.

### A5. Backups

The administrator makes a backup from the menu (option 13). Save it as
`/srv/lms/backups/lms-backup-YYYYMMDD.db`. In addition, set up an automatic daily copy:

```bash
sudo apt install sqlite3
sudo -u lms crontab -e
```

and add this line (backs up at 18:00 every day, keeping one file per weekday):

```
0 18 * * * sqlite3 /srv/lms/lms.db ".backup /srv/lms/backups/auto-$(date +\%a).db" && tar czf /srv/lms/backups/submissions-$(date +\%a).tgz -C /srv/lms submissions
```

Once a week copy the `backups` folder to a USB drive and keep it locked away. The database
backup does **not** include the submitted files, which is why the line above also copies the
`submissions` folder.

To restore, log in as the administrator and use option 14 with the backup file, then put the
matching `submissions` folder back with `tar xzf`.

### A6. Upgrading from 1.0 to 1.1

1. Make a backup (A5).
2. `sudo apt install ./moderated-lms_1.1.0_amd64.deb`
3. Start the program once. It adds the new `class_extension` table to the existing database on
   its own; all existing users, marks and audit records are kept.

### A7. Removing the program

`sudo apt remove moderated-lms`. The data in `/srv/lms` is **not** deleted. Delete it only when
the school's retention period is over: `sudo rm -rf /srv/lms`.

---

## Part B - Windows 10/11 PC (one user at a time)

1. Unzip `moderated-lms-1.1.0-windows.zip` to `C:\LMS`. Nothing else needs installing: `lms.exe`
   contains SQLite and libsodium.
2. Right-click `C:\LMS` → Properties → Security, and remove access for everyone except the
   administrator's Windows account, so other users of the PC cannot copy `lms.db`.
3. Double-click **Start LMS.bat**. The first time, it asks you to create the administrator.
4. For training, **Start demo.bat** opens a separate demo database with sample accounts
   (password `Password1`), and **Reset demo.bat** clears it. The demo never touches `lms.db`.
5. Backups: use menu option 13, then copy the backup file **and** the `C:\LMS\submissions` folder
   to a USB drive.

Windows SmartScreen may warn that the program is from an unknown publisher (it is not signed).
Choose **More info → Run anyway**.

---

## Part C - Building from source

See `README.md` (Ubuntu, MSYS2 and Visual Studio). The release packages are made with:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release && cmake --build build
cd build && cpack -G DEB          # Ubuntu package
bash packaging/build-windows.sh   # Windows zip (needs mingw-w64)
```

---

## Checking the installation

| Check | Expected |
|---|---|
| `lms --help` | Usage text with `--db`, `--files`, `--seed-demo` |
| Log in as the administrator, option 1 | The user list |
| From a lab PC: `ssh lms@<LMS-PC-IP>` | The LMS login screen, not a Linux prompt |
| `ls -l /srv/lms` | Owner `lms`, permissions `drwx------` |
| Option 13 (backup), then option 14 with that file | "Backup saved", then "Data restored" |

## Troubleshooting

| Problem | Cause and fix |
|---|---|
| `Could not open database` | Wrong `--db` path or the folder is not owned by `lms` (redo A2) |
| `This account is locked after 5 wrong passwords` | Wait 15 minutes, or the administrator resets the password (option 4) |
| The administrator forgot their password | Another administrator resets it. If there is none, restore the last backup made while the password was known |
| `database is locked` | Should not happen (busy timeout 5 s). Check the database is on the LMS PC's own disk, not a network or USB drive |
| `ssh` gives a Linux prompt | The `Match User lms` block is missing or SSH was not restarted (A4) |
| Times look wrong | Set the LMS PC's clock and time zone: `sudo timedatectl set-timezone Africa/Maseru` |
