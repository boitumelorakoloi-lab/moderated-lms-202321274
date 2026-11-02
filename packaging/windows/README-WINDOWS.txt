Moderated Learning Management System - Windows package
=======================================================

No installation is needed. lms.exe contains everything it needs (SQLite and libsodium are
built in), and it works with no internet connection.

  Start demo.bat   opens the program with demo accounts - all passwords are Password1:
                   admin, hod.molapo, lect.mokoena, stu.thabo, stu.lerato, stu.palesa
                   A file to try submitting: type demo_files/my_homework.txt when asked
  Reset demo.bat   deletes the demo data so the next demo starts fresh
  Start LMS.bat    opens the program with the real school database (lms.db in this folder).
                   On the first run it asks you to create the administrator account.

Dates are typed as YYYY-MM-DD HH:MM, for example 2026-11-20 14:00.
Full instructions: INSTALL.md (for the technician) and USER_MANUAL.md (for each role).
