# Testing Scripts
 

## Standard hardware check

### When to perform
- after initial boot
- when hardware status is uncertain

### Commands
- get sleep status (0 = awake)
    - if the camera is sleep=1, camera will be unresponsive and most commands will fail untill sleep=0.
- get output mode (5 = yuyv)
    - if output != 5, the output video will look very different
- get autoshutter (0 = disabled)
    - not blocking but useful to know
