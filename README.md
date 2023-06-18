# terminal camera

A webcam client for your terminal, because we surely needed one.

## TODO:

* as of now, it only expects `MJPG` camera format and has to decompress it to
  then process it, the programs should check capabilities and then act
  accordingly. This can cause very low fps or the program simply not working
  with certain (other than mine) hardware
  * currnly to fix it, you can poke the driver with `v4l2-ctl` utility,
    specifically:
    ```
    # lists formats for you to choose one
    v4l2-ctl --list-formats-ext

    # set one of the listed formats, currently only MJPEG will work
    v4l2-ctl -v width=1280,height=720,pixelformat=MJPEG \
    --stream-mmap=3 \
    --stream-count=1000 \
    --stream-to=/dev/null
    ```
* resizing and recoloring can be done while resizing the jpeg, by looking at the
  turbo-jpeg api
