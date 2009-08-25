/* $Id$ */
/* Structures for JSON parsing using only fixed-extent memory */

#include <stdbool.h>
#include <ctype.h>

typedef enum {integer, real, string, boolean, character,
	      object, structobject, array, check} json_type;

#define nullbool	-1	/* not true, not false */

struct json_array_t { 
    json_type element_type;
    union {
	struct {
	    const struct json_attr_t *subtype;
	    char *base;
	    size_t stride;
	} objects;
	struct {
	    char **ptrs;
	    char *store;
	    int storelen;
	} strings;
    } arr;
    int *count, maxlen;
};

struct json_attr_t {
    char *attribute;
    json_type type;
    union {
	int *integer;
	double *real;
	char *string;
	bool *boolean;
	char *character;
	struct json_array_t array;
	size_t offset;
    } addr;
    union {
	int integer;
	double real;
	bool boolean;
	char character;
	char *check;
    } dflt;
    size_t len;
};

#define JSON_ATTR_MAX	31	/* max chars in JSON attribute name */
#define JSON_VAL_MAX	63	/* max chars in JSON value part */

int json_read_object(const char *, const struct json_attr_t *, const char **);
int json_read_array(const char *, const struct json_array_t *, const char **);
const char *json_error_string(int);

#define JSON_ERR_OBSTART	1	/* non-WS when expecting object start */
#define JSON_ERR_ATTRSTART	2	/* non-WS when expecting attrib start */
#define JSON_ERR_BADATTR	3	/* unknown attribute name */
#define JSON_ERR_ATTRLEN	4	/* attribute name too long */
#define JSON_ERR_NOARRAY	5	/* saw [ when not expecting array */
#define JSON_ERR_NOBRAK 	6	/* array element specified, but no [ */
#define JSON_ERR_STRLONG	7	/* string value too long */
#define JSON_ERR_TOKLONG	8	/* token value too long */
#define JSON_ERR_BADTRAIL	9	/* garbage while expecting , or } */
#define JSON_ERR_ARRAYSTART	10	/* didn't find expected array start */
#define JSON_ERR_OBJARR 	11	/* error while parsing object array */
#define JSON_ERR_SUBTOOLONG	12	/* too many array elements */
#define JSON_ERR_BADSUBTRAIL	13	/* garbage while expecting array comma */
#define JSON_ERR_SUBTYPE	14	/* unsupported array element type */
#define JSON_ERR_BADSTRING	15	/* error while string parsing */
#define JSON_ERR_CHECKFAIL	16	/* check attribute not matched */
#define JSON_ERR_BADENUM	17	/* invalid flag token */
#define JSON_ERR_NOPARSTR	18	/* can't support strings in aparalle. arrays */

/* json.h ends here */
