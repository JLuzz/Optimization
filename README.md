# Optimization
501 Assignment 4 Optimization
convert (to frequency) ft(x) = X and ft(h) = H to fft then multiply them together to get Y and convert this result
back to the time domain (by applying some inverse operation) so you can hear it. fft(x) and fft(h) need to be the same
length and a power of 2, so zero pad one of them if they are not equal in size, before passing them into
four1.

Ex. X->zeropad->fft->X (multiply) H -> IFFT ->Y


after four1 you get something like [real, imaginary, real, imaginary] so multiply your array by two to get enough room
for imaginary number
