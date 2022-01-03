#ifndef ESTC_ERROR_H
#define ESTC_ERROR_H

/** @defgroup ESTC_ERRORS_BASE Error Codes Base number definitions
 * @{ */
#define ESTC_ERROR_BASE_NUM      (0x0)       ///< ESTC error base
/** @} */

#define ESTC_SUCCESS                           (ESTC_ERROR_BASE_NUM + 0)  ///< Successful command
#define ESTC_ERROR_NOT_FOUND                   (ESTC_ERROR_BASE_NUM + 1)  ///< Not found
#define ESTC_ERROR_INVALID_PARAM               (ESTC_ERROR_BASE_NUM + 2)  ///< Invalid Parameters
#define ESTC_ERROR_INVALID_PARAM_DATA          (ESTC_ERROR_BASE_NUM + 3)  ///< Invalid Parameters Data
#define ESTC_ERROR_NO_MEM                      (ESTC_ERROR_BASE_NUM + 4)  ///< No Memory for operation
#define ESTC_ERROR_SAME_DATA_NAME              (ESTC_ERROR_BASE_NUM + 5)  ///< The same data name exist
#define ESTC_ERROR_SAME_DATA                   (ESTC_ERROR_BASE_NUM + 6)  ///< The same data exist
#define ESTC_ERROR_NAME_NOT_FOUND              (ESTC_ERROR_BASE_NUM + 7)  ///< Name not found

typedef uint8_t estc_ret_code_t;

#endif // ESTC_ERROR_H