/*****************************************************************************
 *
 * Original Author: 
 * Creation Date:   
 * Description: 
 *
 ****************************************************************************/

/*****************************************************************************
 *                                INCLUDE FILES 
 *****************************************************************************/
#include "bitd/types.h"
#include "bitd/base64.h"


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
 *                           FUNCTION DECLARATION
 *****************************************************************************/



/*****************************************************************************
 *                                VARIABLES
 *****************************************************************************/

/* Ascii table */
static const char bin2a[] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

/* Reverse of the above - the value at a2bin['x'] is the index of 'x'
   in the above array, or 64 if not found in the table */
static const unsigned char a2bin[256] = {
    /* ASCII table */
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 62, 64, 64, 64, 63,
    52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 64, 64, 64, 64, 64, 64,
    64,  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, 14,
    15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 64, 64, 64, 64, 64,
    64, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40,
    41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64,
    64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64, 64
};

/*****************************************************************************
 *                          FUNCTION IMPLEMENTATION
 *****************************************************************************/


/*
 *============================================================================
 *                        bitd_base64_decode_len
 *============================================================================
 * Description:     Get length of buffer once decoded,
 *     including NULL termination
 * Parameters:    
 * Returns:  
 */
int bitd_base64_decode_len(const char *bufcoded) {
    int nbytesdecoded;
    register const unsigned char *bufin;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (a2bin[*(bufin++)] <= 63);

    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    return nbytesdecoded + 1;
}


/*
 *============================================================================
 *                        bitd_base64_decode
 *============================================================================
 * Description:     Decode the buffer
 * Parameters:    
 * Returns:  
 *     Length of decoded buffer including NULL termination
 */
int bitd_base64_decode(char *bufplain, const char *bufcoded) {
    int nbytesdecoded;
    register const unsigned char *bufin;
    register unsigned char *bufout;
    register int nprbytes;

    bufin = (const unsigned char *) bufcoded;
    while (a2bin[*(bufin++)] <= 63);
    nprbytes = (bufin - (const unsigned char *) bufcoded) - 1;
    nbytesdecoded = ((nprbytes + 3) / 4) * 3;

    bufout = (unsigned char *) bufplain;
    bufin = (const unsigned char *) bufcoded;

    while (nprbytes > 4) {
        *(bufout++) =
            (unsigned char) (a2bin[*bufin] << 2 | a2bin[bufin[1]] >> 4);
        *(bufout++) =
            (unsigned char) (a2bin[bufin[1]] << 4 | a2bin[bufin[2]] >> 2);
        *(bufout++) =
            (unsigned char) (a2bin[bufin[2]] << 6 | a2bin[bufin[3]]);
        bufin += 4;
        nprbytes -= 4;
    }

    /* Note: (nprbytes == 1) would be an error, so just ingore that case */
    if (nprbytes > 1) {
        *(bufout++) =
            (unsigned char) (a2bin[*bufin] << 2 | a2bin[bufin[1]] >> 4);
    }
    if (nprbytes > 2) {
        *(bufout++) =
            (unsigned char) (a2bin[bufin[1]] << 4 | a2bin[bufin[2]] >> 2);
    }
    if (nprbytes > 3) {
        *(bufout++) =
            (unsigned char) (a2bin[bufin[2]] << 6 | a2bin[bufin[3]]);
    }

    *(bufout++) = '\0';
    nbytesdecoded -= (4 - nprbytes) & 3;
    return nbytesdecoded;
}


/*
 *============================================================================
 *                        bitd_base64_encode_len
 *============================================================================
 * Description:     Get length of buffer once encoded,
 *     including NULL termination
 * Parameters:    
 * Returns:  
 */
int bitd_base64_encode_len(int len) {
    return ((len + 2) / 3 * 4) + 1;
}


/*
 *============================================================================
 *                        bitd_base64_encode
 *============================================================================
 * Description:     Encode the buffer
 * Parameters:    
 * Returns:  
 *     Length of encoded buffer including NULL termination
 */
int bitd_base64_encode(char *bufcoded, const char *bufplain, int len) {
    int i;
    char *p;

    p = bufcoded;
    for (i = 0; i < len - 2; i += 3) {
        *p++ = bin2a[(bufplain[i] >> 2) & 0x3F];
        *p++ = bin2a[((bufplain[i] & 0x3) << 4) |
                     ((int) (bufplain[i + 1] & 0xF0) >> 4)];
        *p++ = bin2a[((bufplain[i + 1] & 0xF) << 2) |
                     ((int) (bufplain[i + 2] & 0xC0) >> 6)];
        *p++ = bin2a[bufplain[i + 2] & 0x3F];
    }
    if (i < len) {
        *p++ = bin2a[(bufplain[i] >> 2) & 0x3F];
        if (i == (len - 1)) {
            *p++ = bin2a[((bufplain[i] & 0x3) << 4)];
            *p++ = '=';
        }
        else {
            *p++ = bin2a[((bufplain[i] & 0x3) << 4) |
                         ((int) (bufplain[i + 1] & 0xF0) >> 4)];
            *p++ = bin2a[((bufplain[i + 1] & 0xF) << 2)];
        }
        *p++ = '=';
    }

    *p++ = '\0';
    return p - bufcoded;
}
