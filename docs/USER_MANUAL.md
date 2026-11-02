# User Manual

Moderated Learning Management System · release 1.1.0

This manual has one part for each kind of user. Read **Part 1** first, then your own part.

| Part | For | Main jobs |
|---|---|---|
| 1 | Everyone | Logging in, moving around, changing your password |
| 2 | Students | Seeing your work, handing it in, reading your marks |
| 3 | Lecturers | Creating assignments, extensions, marking |
| 4 | Heads of Department | Checking (moderating) marks, releasing them, the audit log |
| 5 | Administrators | Accounts, courses, enrolments, backups |

---

## Part 1 - Everyone

### Logging in

1. Start the program (your technician will show you how: a desktop shortcut or `ssh` command).
2. Type your **username** and press Enter, then your **password** (it is not shown while you
   type) and press Enter.
3. Your own menu opens. You only see the options your role is allowed to use.

If you type the wrong password 5 times in a row, your account is **locked for 15 minutes**. Wait,
or ask the administrator to reset your password.

### Moving around

- Type the **number** of an option and press Enter.
- `0` closes the current menu and logs you out. At the `Username` prompt, press Enter on its own to
  close the program.
- Dates and times are typed as `YYYY-MM-DD HH:MM`, for example `2026-11-20 14:00`.
- When the program asks for an ID (assignment, submission, student), the number is shown in the
  first column of the lists.
- Typing something the program does not expect is safe: it shows `ERROR: ...` and asks again.

### Changing your password

Choose `P` in your menu, type your current password, then the new one twice. Passwords must have
at least 8 characters. The administrator never sees your password - it is stored only as a
scrambled code (hash).

---

## Part 2 - Students

| Option | What it does |
|---|---|
| 1. My courses and assignments | Your courses, then every assignment with its deadline, maximum mark and state |
| 2. Submit an assignment | Hand in a file |
| 3. My released marks and feedback | Marks that your Head of Department has released |

### Seeing your assignments (option 1)

The **State** column shows where your work is: `Not submitted`, `Submitted`, `Late`, `Marked`,
`Returned`, `Moderated` or `Released`. You can only see a mark once it is `Released`.

A deadline with a `*` has been **extended**. Under the list, *Extended deadlines* shows for each
one whether it is a **whole-class** extension or a **personal** one, the old and the new
deadline, and the lecturer's reason. If you have both, the later deadline counts.

### Handing in work (option 2)

1. Type the assignment ID.
2. Type the path of your file, for example `C:\Users\thabo\Documents\algebra.pdf` or
   `demo_files/my_homework.txt`. Allowed: `.pdf`, `.docx`, `.txt`, `.zip`, up to 10 MB, not empty.
3. The program shows **your** deadline (with any extension) and tells you if the work is on time.
4. Type `y` to hand it in. You get a **receipt** with a submission and version number - write it
   down.

Rules:

- Before the deadline you can hand in again as often as you like. Only the latest version is
  marked; older versions are kept.
- After the deadline the program still accepts the work, but warns you that it is late. Late work
  loses **10% of the maximum mark for each day** (or part of a day) late. More than 3 days late
  gives 0. Late work can be handed in only once.
- Example: maximum 50, raw mark 40, handed in 1 hour late → 1 day late → 40 − 5 = **35**.

### Reading your marks (option 3)

For each released assignment you see the raw mark, days late and penalty, the final mark, and
the feedback.

---

## Part 3 - Lecturers

| Option | What it does |
|---|---|
| 1. My courses | The courses you teach, with their IDs |
| 2. Create an assignment | Title, deadline and maximum mark (1-100) |
| 3. List assignments for a course | Assignments with deadline, any whole-class extension, and whether marks are released |
| 4. Give a student an extension | A later deadline for one student |
| 5. Mark submissions | Mark, re-mark, and see work returned by the HoD |
| 6. Extend the deadline for the whole class | A later deadline for every student on one assignment |

### Creating an assignment (option 2)

Type the course ID (from option 1), a title, the deadline and the maximum mark.

### Extensions

**One student (option 4):** type the assignment ID, the student's ID, the new deadline and a
reason. The new deadline must be after the normal one. You cannot give an extension once the
work has been marked. If the student's late work is covered by the new deadline, it becomes
on time.

**Whole class (option 6):** type the assignment ID, the new deadline and a reason - students see
the reason on their list, so keep it short and suitable (for example *Exam week clash*).

- The new deadline must be after the normal deadline, and later than any earlier whole-class
  extension (it can only move later, never earlier).
- It is refused once marking has started on that assignment. Use individual extensions then.
- Late work that the new deadline covers becomes on time, and the program tells you how many.
- A student who already has a later personal extension keeps it.

### Marking (option 5)

1. Type the assignment ID. The list shows each submission with its state, days late and file.
2. Type the submission ID to mark.
3. Type the raw mark (0 to the maximum) and written feedback (required).
4. The program shows the final mark after the late penalty.

To **change** a mark, mark the same submission again: you must give a reason, and the change is
written to the audit log. Work the HoD has **returned** to you shows the HoD's comment; re-mark
it and it goes back to the HoD. You cannot change a mark after the HoD has approved it.

---

## Part 4 - Heads of Department

| Option | What it does |
|---|---|
| 1. Assignments in my department | Every assignment in your department's courses |
| 2. Moderate marks for an assignment | Approve, adjust or return each mark |
| 3. Release marks for an assignment | Let students see the marks |
| 4. View the audit log for a submission | Every mark change: who, when, old, new, why |
| 5. Open my lecturer menu | Your own teaching (exit it to come back) |

### Moderating (option 2)

Type the assignment ID. For each submission you see the raw mark, days late, penalty, final mark,
feedback and any extension (`class` in front means a whole-class extension). Type a submission
ID, then choose:

1. **Approve** - the mark is correct.
2. **Adjust the mark** - type the new raw mark and a reason (required).
3. **Return to the lecturer** - type a comment saying what to check. The lecturer must re-mark it.

Every action is saved in the audit log and cannot be deleted.

### Releasing (option 3)

You can release only when **every** submission of that assignment is approved. The program says
how many still need approval. After you confirm, students see their marks straight away, and
marks can no longer be changed.

---

## Part 5 - Administrators

| Option | What it does |
|---|---|
| 1 / 2 / 3 | List users, create a user, edit a user's name |
| 4 / 5 | Reset a password, deactivate a user |
| 6 / 7 / 8 | List departments and courses, create a department, set its HoD |
| 9 / 10 / 11 | Create a course, assign a lecturer, enrol a student |
| 12 | View the audit log for a submission |
| 13 / 14 | Back up data, restore data |

**Setting up a new term:** create departments (7) → create the HoD accounts (2, role HoD) and set
each department's HoD (8) → create courses (9) → create lecturer and student accounts (2) →
assign lecturers (10) → enrol students (11).

- Usernames are unique and have at least 3 characters. New passwords need at least 8.
- **Deactivate** (5) instead of deleting: the person can no longer log in, but their marks and the
  audit log stay complete.
- **Reset password** (4) sets a new password and unlocks the account. Tell the person to change it
  with `P` at once.

**Backups (13):** type a file name such as `/srv/lms/backups/lms-backup-20261102.db`. Ask the
technician to copy backups, together with the `submissions` folder, to a USB drive kept locked
away.

**Restore (14):** replaces **all** current data with the backup. Everyone must log out first.
Files that are not LMS backups are refused and nothing is changed.

---

## Messages you may see

| Message | Meaning |
|---|---|
| `Wrong username or password.` | Check both. After 5 tries the account locks for 15 minutes |
| `The deadline has passed. This work will be N day(s) late` | Late penalty applies (Part 2) |
| `Late work can only be handed in once` | You already handed in late work for this assignment |
| `A reason is required ...` | Mark changes, adjustments, returns and extensions need a reason |
| `N mark(s) still need approval.` | Release is only possible when all marks are approved |
| `Marking has already started, so the class deadline can no longer be moved.` | Use individual extensions |
| `You are not allowed to ...` / `You do not teach ...` | The action is not part of your role |
