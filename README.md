# Open Source Media Center

OSMC (Open Source Media Center) is a free and open source media player based on Linux. Founded in 2014, OSMC lets you play back media from your local network, attached storage and the Internet. OSMC is the leading media center in terms of feature set and community and is based on the Kodi project. You can learn more about OSMC at <a href="http://osmc.tv/about">osmc.tv/about</a>. 

#### Source Code ####

Upstream source code for OSMC is available at https://github.com/osmc/osmc. You can clone this repository with the command 'git clone'. You can learn about how to set up a development environment <a href="https://osmc.tv/wiki/development/getting-involved-with-osmc-development/">here</a>.

#### Contributing ####

After cloning, run `sh hooks/install.sh` once. It points git at this repository's `hooks/` directory, so the hooks arrive with a clone and update with a pull. They check that a commit subject carries the area prefix this repository uses (for example `[package] [base-files-osmc]`), warn before pushing a package whose version was not bumped, and strip build output such as `Installed-Size` out of `DEBIAN/control` files. The hooks are advisory and run only on your own machine, so this needs doing once per clone.

#### Download ####

A compiled version of OSMC can be downloaded from <a href="https://osmc.tv/download">https://osmc.tv/download</a>. 

#### Issues ####

Before creating a new GitHub issue, please read our policy on <a href="https://osmc.tv/wiki/development/reporting-bugs/">reporting bugs</a>. This will ensure that developer time is used effectively, and your issue is dealt with promptly, whether it is indeed a bug or not. 

#### License ####

OSMC is licensed under the GPLv2. Commits to the OSMC codebase become copyright of OSMC so that it can be maintained and relicensed (i.e. -> GPLv3) if necessary without having to maintain contact with the author. OSMC does not claim to own upstream code, and recommends you consult the license of those projects for further information.

#### Help and support ####

For help and support, please refer to our blog at <a href="https://osmc.tv">https://osmc.tv</a>. We have a discussion forum at <a href="https://discourse.osmc.tv">https://discourse.osmc.tv</a> with a bustling community full of users eager to help each other out. 
