#get fps of vid
ffprobe -v error -select_streams v:0 -show_entries stream=r_frame_rate,avg_frame_rate -of default=noprint_wrappers=1 video.mp4
#split
ffmpeg -i video.mp4 frames/frame_%05d.png
