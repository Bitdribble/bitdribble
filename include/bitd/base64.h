#ifndef _BITD_BASE64_H_
#define _BITD_BASE64_H_

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/platform-types.h"


#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

/*****************************************************************************
 *                             MANIFEST CONSTANTS
 *****************************************************************************/



/*****************************************************************************
 *                                  MACROS 
 *****************************************************************************/



/*****************************************************************************
 *                                  TYPES
 *****************************************************************************/



/*****************************************************************************
 *                            FUNCTION DEFINITIONS
 *****************************************************************************/

int bitd_base64_decode_len(const char *bufcoded);
int bitd_base64_decode(char *bufplain, const char *bufcoded);
int bitd_base64_encode_len(int len);
int bitd_base64_encode(char *bufcoded, const char *bufplain, int len);


#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _BITD_BASE64_H_ */
