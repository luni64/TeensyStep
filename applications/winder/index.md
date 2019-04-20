---
title: Coil Winder

permalink: /applications/winder/
---

## Overview

The following figure shows the principal setup of a coil winder. A string (blue) is rotated by the spindle motor. While the spindle rotates the feeder (green) linearly moves parallel to the string and winds a wire on the string. It is important that the ratio of the feeder speed and the spindle rotation stays constant even during acceleration and deceleration phases to keep a constant winding pitch. 


![Overview](assets/winder.png)

### Simple Approach
Since both StepControl and RotateControl are able to rotate motors in  sync a quick solution to the problem is easily done. 

```c++
// code tbd
```

The shown code periodically prints out the current speed of the feeder and spindle motor and the ratio (spindle speed)/(feeder speed). 

IMAGE
[newfile](newpage/)
[newfile](newpage)


TBD....

