# 457

https://github.com/cthrax/cs457/tree/master/a2

In the C programming language, the select system call is declared in the header file sys/select.h or unistd.h, and has the following syntax:

int select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *errorfds, struct timeval *timeout);

nfds: This is an integer one more than the maximum of any file descriptor in any of the sets. In other words, while adding file descriptors to each of the sets, you must calculate the maximum integer value of all of them, then increment this value by one, and then pass this as nfds.

readfds: fd_set type holding the file descriptors to be checked for being ready to read, and on output indicates which file descriptors are ready to read. Can be NULL.

writefds: fd_set type holding the file descriptors to be checked for being ready to write, and on output indicates which file descriptors are ready to write. Can be NULL.

errorfds: fd_set type holding the file descriptors to be checked for error conditions pending, and on output indicates which file descriptors have error conditions pending. Can be NULL.

timeout: structure of type struct timeval that specifies a maximum interval to wait for the selection to complete. If the timeout argument points to an object of type struct timeval whose members are 0, select() does not block. If the timeout argument is NULL, select() blocks until an event causes one of the masks to be returned with a valid (non-zero) value. Linux will update the timeout in place to indicate how much time was elapsed, though this behavior is not shared by most other Unix systems.


fd_set type arguments may be manipulated with four utility macros: FD_SET(), FD_CLR(), FD_ZERO(), and FD_ISSET().

http://man7.org/linux/man-pages/man2/select.2.html

