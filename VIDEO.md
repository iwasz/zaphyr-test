This example generates 64x64 still video stream. Best viewed with a command such as:

```sh
gst-launch-1.0 udpsrc port=5000 ! videoparse format=rgb16 width=64 height=64 ! queue ! videoconvert ! fpsdisplaysink sync=false
```
