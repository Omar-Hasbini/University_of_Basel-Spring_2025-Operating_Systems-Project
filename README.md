Installation:
Deliverables for origin feature: ~/daemon prod

Dependencies:
clang
libbpf-dev
libelf-dev
zlib1g-dev
llvm-12-tools
sqlite3
build-essential
linux-headers-$(uname -r)

when in folder /daemon prod, simply run make

To start the daemons: sudo ./daemon && sudo ./daemon_download_origin

Final commit: a97e6f052924dc116e9232c0eb5d3f3b1aad23a6
