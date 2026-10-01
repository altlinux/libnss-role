# libnss-role [![Coverity Scan Build Status](https://scan.coverity.com/projects/etersoft-libnss-role/badge.svg)](https://scan.coverity.com/projects/etersoft-libnss-role)

NSS API library and admin tools for roles and privileges.
This README is also available in [Russian (Русский)](README-ru.md)

## Contents

* [Introduction](#introduction)
* [What is this?](#whatisthis)
* [Build](#build)
* [Administration](#administration)

* * *

## Introduction

**libnss-role** is an NSS libc module that implements adding groups to
groups for Linux / Unix systems and also provides functionality similar to
Windows' "Restricted Groups".


## What is this?

`libnss_role` is an [NSS](https://en.wikipedia.org/wiki/Name_Service_Switch) module adds support for adding groups to groups.
This makes certain system administration tasks easier.

For example, you can create a group named `users` and add it to groups such as `audio`, `cdwriter`, `serial` etc.
Now you can add all users to `users` group instead of adding them to all these groups manually.

Furthermore, you can create a group named `admins` and add it to groups `users` and `wheel`. As a result, administrators
will get all "user" groups and a `wheel` group in addition.

In other words, `libnss-role` allows to achieve the following:

* user `vasya` is a member of the group `users`;
* we want all users-members of the group `users` to become members of the group `cdrom` automatically, but don't want to manually add each member of `users` to `cdrom`;
* `libnss-role` does so that when `glibc` is asked to provide groups that the user belongs to, the answer is "modified" as described above.

In GNU/Linux distributions, practically all programs and libraries are linked with system `libc.so.6` (glibc) and this means that if they use standard methods to get information about users, `libnss-role` will work. If a binary is statically linked with any libc or does not use the system glibc in other ways, `libnss-role` will not work for this binary.

This module has its own administration utilities. These utilities divide all groups into two categories: **roles** and **privileges**.

### Privilege
This is an ordinary POSIX group that can be assigned to a user.
Members of a "privilege" group can perform certain actions.
Examples of privileges are: `cdwriter`, `audio`, `serial`, `virtualbox` and many others.

### Role
A role is also a POSIX group, but it has another meaning: it describes a type of activity that a user performs.
Examples of roles are: `admin`, `user`, `power_user`, `developer` and others.

A user may need some special rights to perform his role. It is useful to have a possibility of adding sets of privileges to
users indirectly via assigning a role. For example, an user with `admin` role could also get privilege groups such as
`wheel`, `ssh`, `root` etc.

This module implements such permission management model.

## Build

### Dependencies

* **CMake 3.10+** and a C compiler for building
* **PAM** development files (`libpam0-devel` in ALT, `pam-devel` or `libpam0g-dev` elsewhere)
* **cmocka** and **nss_wrapper** for unit testing (`-DENABLE_TESTS=OFF` to build without tests)

The build process is as simple as:

```
mkdir build
cd build
cmake ..
make
ctest
```

Unit tests are also run by `make`. To run them under valgrind too, configure with
`-DENABLE_TESTS_VALGRIND=ON`.

Install directories can be changed with `-DNSS_LIBDIR=`, `-DROLE_LIBDIR=` and `-DMANDIR=` (for man8 pages).

Now you need to enable the module. Open `/etc/nsswitch.conf` and append `role` to the end of the line that starts with `group:`.
You should get something like this:
```
group: files ldap role
```

## Administration

### Configuration files
This module uses `/etc/role` file, files in `/etc/role.d` directory and the system group database (`/etc/group`, LDAP, winbind...) to store role information.
The format is described in `role(5)`.

`/etc/role` stores additional information about groups that are included in other groups.
Format of `/etc/role` is as follows:
```
<role>:<group>[,<group>]*
```
where `<role>` and `<group>` are group names or numeric group identifiers.

A group before `:` is a role and its members will be included in other groups.
Groups after `:` are groups that members of this role get.
Nested groups are resolved recursively.

* Group names may contain spaces (e.g. `domain users`) and may be enclosed in double quotes.
* `#` starts a comment.
* Several lines for the same role are merged.
* Groups which don't exist are skipped.

Here is an example. Suppose that we have a user named `pupkin` and we have some records in `/etc/group`:
```
group1:x:1:pupkin
group2:x:2:pupkin
group3:x:3:
group4:x:4:
group5:x:5:
group6:x:6:
```

Meanwhile the `/etc/role` file contains:
```
group2:group3,group4
group4:group5,group6
```

With such configuration `pupkin` will get all the groups.
* he gets `group1` and `group2` as they are assigned to him directly;
* he gets `group3` and `group4` as they are assigned to `group2`;
* he gets `group5` and `group6` as they are assigned to `group4`.

The same configuration with numeric identifiers is `2:3,4` and `4:5,6`.

Additional configuration, e.g. installed by packages, can be placed into `/etc/role.d/*.role` files.
They are read after `/etc/role` and merged with it.
A file `/etc/role.d/ROLE.role` describing the role `ROLE` is a *system role* file.

### Administration utilities
There are three utilities: `roleadd`, `roledel` and `rolelst`.
By default `roleadd` and `roledel` change `/etc/role`. With `-f FILE.role` they change `/etc/role.d/FILE.role`,
with `-S` they change the system role file `/etc/role.d/ROLE.role`.

#### roleadd
```
roleadd [-s] [-m] [-f FILE.role | -S] ROLE [GROUP*]
```

Adds a role (if not exists) and assigns privileges and roles to it.

`ROLE` is a role name. Must match an existing group name.

`GROUP` is a name of role or a privilege.

When used with `-s` switch the groups are set; by default groups are appended.
With `-m` missing groups are skipped instead of an error.

#### roledel
```
roledel [-m] [-f FILE.role | -S] ROLE [GROUP*]
```
or
```
roledel -r [-f FILE.role | -S] ROLE
```

Used to delete privileges from roles and to delete roles themselves (second form).

#### rolelst
```
rolelst [-n] [-V] [-f FILE.role | -S [ROLE]] [ROLE*]
```

Shows roles from `/etc/role` and `/etc/role.d` with group names (`-n` shows numeric identifiers).
If `ROLE`s are given, only these roles are shown.
`-f FILE.role` shows roles from one file, `-S` shows system roles, `-V` shows `/etc/role` and the merged configuration separately.
