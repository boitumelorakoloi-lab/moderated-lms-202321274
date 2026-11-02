# Release Notes

Moderated Learning Management System · CS4433 · Boitumelo Rakoloi (202321274)

Versions follow Semantic Versioning: MAJOR.MINOR.PATCH. A MINOR release adds features without
breaking existing data.

---

## v1.0.0 - 2 November 2026

First release for the school.

### Features

- Log in with username and password; Argon2id password hashes (libsodium); account locked for
  15 minutes after 5 wrong passwords; change own password.
- **Administrators:** user accounts (create, edit, reset password, deactivate), departments and
  HoDs, courses, lecturer assignment, enrolment, audit log, backup and restore.
- **Lecturers:** create assignments, give one student an extension, mark with feedback, re-mark
  with a reason.
- **Students:** see courses and assignments, hand in files (.pdf, .docx, .txt, .zip, up to
  10 MB), resubmit before the deadline, see released marks and feedback.
- **Heads of Department:** moderate (approve, adjust with a reason, return to the lecturer),
  release marks once all are approved, view the audit log.
- Late penalty: 10% of the maximum mark per day late (part days count as a full day), 0 after
  3 days. Work handed in exactly at the deadline is on time.
- Append-only audit log of every mark change (who, when, old mark, new mark, reason).
- Works offline; SQLite database in WAL mode for up to 20 users at once.

### Packages

- Ubuntu 24.04: `moderated-lms_1.0.0_amd64.deb`
- Windows 10/11: `moderated-lms-1.0.0-windows.zip` (stand-alone `lms.exe`, no installer)

### Known limitations

- Console (text) interface only.
- Students hand in files that are on the LMS PC; from a lab PC the file must be copied there
  first (Installation Guide, A4).
- The backup made from the menu covers the database; the `submissions` folder is backed up by
  the technician's daily job (Installation Guide, A5).
- The Windows `lms.exe` is not code-signed, so SmartScreen shows a warning.
