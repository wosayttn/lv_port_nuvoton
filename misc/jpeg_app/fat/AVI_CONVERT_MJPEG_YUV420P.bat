.\bin\ffmpeg -i nuvoton.avi -vf scale=480:272 -c:v mjpeg -q:v 2 -pix_fmt yuv420p -an ./root/movie.avi
