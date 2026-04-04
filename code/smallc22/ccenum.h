// ccenum.h -- Enum table layout for Small-C compiler.

// Enum tag table: tracks defined enum tag names so that
// "enum tag var;" (use without body) can be validated.
// Each entry is a fixed-size slot storing the tag name.
#define EDAT_NAME   0       // tag name, null-terminated, up to NAMEMAX chars
#define EDAT_MAX   14       // NAMEMAX(12) + null(1) + pad(1) = 14

#define EDAT_NUM   20       // max distinct enum tags
#define EDAT_START  enumdata
#define EDAT_END    (enumdata + EDAT_NUM * EDAT_MAX)
#define ENUMDATSZ   (EDAT_NUM * EDAT_MAX)   // 280 bytes
