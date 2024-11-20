1. Install PuTTY from here: https://www.putty.org/
2. Set up data server in PuTTY by typing in username@data.cs.purdue.edu, and in the ssh section under X11 click the box that says enable X11 forwarding. Save that session.
3. Install xming from here: https://sourceforge.net/projects/xming/
4. Use the setup and configure as default through PuTTY (it will be clear when going through setup basically don't change any settings).
5. Right now, the project structure is:
- Project-6-Task-Manager
  - Makefile
  - src
    - main.c
6. Add your edits in src for the part you are working on
7. To test:
```
make clean
make
./system_monitor
```
8. If you did the previous steps correctly it should work (not that this is for Windows machines, if you have a Mac, check this ed discussion post and look at the comment from Sage Stefonic: https://edstem.org/us/courses/62297/discussion/5728154
