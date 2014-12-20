/* Output of mkerrcodes.awk.  DO NOT EDIT.  */

static struct
  {
    int err;
    const char *err_sym;
  } err_table[] = 
{
  { 7, "GPG_ERR_E2BIG" },
  { 13, "GPG_ERR_EACCES" },
  { (10000 + 13), "GPG_ERR_EACCES" },
  { 100, "GPG_ERR_EADDRINUSE" },
  { (10000 + 48), "GPG_ERR_EADDRINUSE" },
  { 101, "GPG_ERR_EADDRNOTAVAIL" },
  { (10000 + 49), "GPG_ERR_EADDRNOTAVAIL" },
  { 102, "GPG_ERR_EAFNOSUPPORT" },
  { (10000 + 47), "GPG_ERR_EAFNOSUPPORT" },
  { 11, "GPG_ERR_EAGAIN" },
  { 103, "GPG_ERR_EALREADY" },
  { (10000 + 37), "GPG_ERR_EALREADY" },
  { 9, "GPG_ERR_EBADF" },
  { (10000 + 9), "GPG_ERR_EBADF" },
  { 16, "GPG_ERR_EBUSY" },
  { 105, "GPG_ERR_ECANCELED" },
  { 10, "GPG_ERR_ECHILD" },
  { 106, "GPG_ERR_ECONNABORTED" },
  { (10000 + 53), "GPG_ERR_ECONNABORTED" },
  { 107, "GPG_ERR_ECONNREFUSED" },
  { (10000 + 61), "GPG_ERR_ECONNREFUSED" },
  { 108, "GPG_ERR_ECONNRESET" },
  { (10000 + 54), "GPG_ERR_ECONNRESET" },
  { 36, "GPG_ERR_EDEADLK" },
  { 36, "GPG_ERR_EDEADLOCK" },
  { 109, "GPG_ERR_EDESTADDRREQ" },
  { (10000 + 39), "GPG_ERR_EDESTADDRREQ" },
  { 33, "GPG_ERR_EDOM" },
  { (10000 + 69), "GPG_ERR_EDQUOT" },
  { 17, "GPG_ERR_EEXIST" },
  { 14, "GPG_ERR_EFAULT" },
  { (10000 + 14), "GPG_ERR_EFAULT" },
  { 27, "GPG_ERR_EFBIG" },
  { (10000 + 64), "GPG_ERR_EHOSTDOWN" },
  { 110, "GPG_ERR_EHOSTUNREACH" },
  { (10000 + 65), "GPG_ERR_EHOSTUNREACH" },
  { 42, "GPG_ERR_EILSEQ" },
  { 112, "GPG_ERR_EINPROGRESS" },
  { (10000 + 36), "GPG_ERR_EINPROGRESS" },
  { 4, "GPG_ERR_EINTR" },
  { (10000 + 4), "GPG_ERR_EINTR" },
  { 22, "GPG_ERR_EINVAL" },
  { (10000 + 22), "GPG_ERR_EINVAL" },
  { 5, "GPG_ERR_EIO" },
  { 113, "GPG_ERR_EISCONN" },
  { (10000 + 56), "GPG_ERR_EISCONN" },
  { 21, "GPG_ERR_EISDIR" },
  { 114, "GPG_ERR_ELOOP" },
  { (10000 + 62), "GPG_ERR_ELOOP" },
  { 24, "GPG_ERR_EMFILE" },
  { (10000 + 24), "GPG_ERR_EMFILE" },
  { 31, "GPG_ERR_EMLINK" },
  { 115, "GPG_ERR_EMSGSIZE" },
  { (10000 + 40), "GPG_ERR_EMSGSIZE" },
  { 38, "GPG_ERR_ENAMETOOLONG" },
  { (10000 + 63), "GPG_ERR_ENAMETOOLONG" },
  { 116, "GPG_ERR_ENETDOWN" },
  { (10000 + 50), "GPG_ERR_ENETDOWN" },
  { 117, "GPG_ERR_ENETRESET" },
  { (10000 + 52), "GPG_ERR_ENETRESET" },
  { 118, "GPG_ERR_ENETUNREACH" },
  { (10000 + 51), "GPG_ERR_ENETUNREACH" },
  { 23, "GPG_ERR_ENFILE" },
  { 119, "GPG_ERR_ENOBUFS" },
  { (10000 + 55), "GPG_ERR_ENOBUFS" },
  { 19, "GPG_ERR_ENODEV" },
  { 2, "GPG_ERR_ENOENT" },
  { 8, "GPG_ERR_ENOEXEC" },
  { 39, "GPG_ERR_ENOLCK" },
  { 12, "GPG_ERR_ENOMEM" },
  { 123, "GPG_ERR_ENOPROTOOPT" },
  { (10000 + 42), "GPG_ERR_ENOPROTOOPT" },
  { 28, "GPG_ERR_ENOSPC" },
  { 40, "GPG_ERR_ENOSYS" },
  { 126, "GPG_ERR_ENOTCONN" },
  { (10000 + 57), "GPG_ERR_ENOTCONN" },
  { 20, "GPG_ERR_ENOTDIR" },
  { 41, "GPG_ERR_ENOTEMPTY" },
  { (10000 + 66), "GPG_ERR_ENOTEMPTY" },
  { 128, "GPG_ERR_ENOTSOCK" },
  { (10000 + 38), "GPG_ERR_ENOTSOCK" },
  { 129, "GPG_ERR_ENOTSUP" },
  { 25, "GPG_ERR_ENOTTY" },
  { 6, "GPG_ERR_ENXIO" },
  { 130, "GPG_ERR_EOPNOTSUPP" },
  { (10000 + 45), "GPG_ERR_EOPNOTSUPP" },
  { 132, "GPG_ERR_EOVERFLOW" },
  { 1, "GPG_ERR_EPERM" },
  { (10000 + 46), "GPG_ERR_EPFNOSUPPORT" },
  { 32, "GPG_ERR_EPIPE" },
  { (10000 + 67), "GPG_ERR_EPROCLIM" },
  { 134, "GPG_ERR_EPROTO" },
  { 135, "GPG_ERR_EPROTONOSUPPORT" },
  { (10000 + 43), "GPG_ERR_EPROTONOSUPPORT" },
  { 136, "GPG_ERR_EPROTOTYPE" },
  { (10000 + 41), "GPG_ERR_EPROTOTYPE" },
  { 34, "GPG_ERR_ERANGE" },
  { (10000 + 71), "GPG_ERR_EREMOTE" },
  { 30, "GPG_ERR_EROFS" },
  { (10000 + 58), "GPG_ERR_ESHUTDOWN" },
  { (10000 + 44), "GPG_ERR_ESOCKTNOSUPPORT" },
  { 29, "GPG_ERR_ESPIPE" },
  { 3, "GPG_ERR_ESRCH" },
  { (10000 + 70), "GPG_ERR_ESTALE" },
  { 138, "GPG_ERR_ETIMEDOUT" },
  { (10000 + 60), "GPG_ERR_ETIMEDOUT" },
  { (10000 + 59), "GPG_ERR_ETOOMANYREFS" },
  { (10000 + 68), "GPG_ERR_EUSERS" },
  { 140, "GPG_ERR_EWOULDBLOCK" },
  { (10000 + 35), "GPG_ERR_EWOULDBLOCK" },
  { 18, "GPG_ERR_EXDEV" },
};
