ECE-385-Final-Project


===

For our ECE385 Final Project, we implemented 1v1 Tetris on the FPGA. To run this game, use mb\_usb\_hdmi\_top.xsa as the hardware platform in vitis. Add the helloworld.c program in the workspace2/tetris/src folder, add the necessary dependencies. Then, connect the FPGA board and two usb keyboards to a USB hub, connect the usb hub to your computer, and connect the HDMI port of the FPGA. Then, run the vitis program. After that, in /KeyboardInput, run the combined\_ver.py file for keyboard inputs.

To start the game, first both keyboards are paired by pressing enter. Then, users select mods :

Q: No Holding

W: Faster Gravity
E: Messy Garbage
R: No Garbage
T: Single player



Any combination of mods can be set. Then, both users press enter to confirm they are ready to play!



Controls:

Right: Move piece right

Left: Move piece left

Up: Rotate CW

Down: Soft drop

Space: Hard drop

C: Store

Z: Rotate CCW



Lastly, keyboard input is currently limited to tapping keys, so holding will not work.





