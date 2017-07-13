#ifndef __GOODIX_INTERFACE_H__
#define __GOODIX_INTERFACE_H__

/**********************************
 *      Header File inclusion     *
 **********************************/



/**********************************
 *        Macro Definition        *
 **********************************/



/**********************************
 *       Typedef Declaration      *
 **********************************/
typedef void (*notify_callback)(int, int, int);



/**********************************
 *      Function Declaration      *
 **********************************/

/******************************************************************************
 *  Description: Init fingerprint sensor
 *
 *  Param: notify_callback function
 *
 *  Return: 0 success, -1 failure
 *
 *****************************************************************************/
int fingerprint_init(notify_callback notify);

/*****************************************************************************
 *  Description: Close fingerprint sensor
 *
 *  Param: NULL
 *
 *  Return: 0 success, -1 failure
 *
 *****************************************************************************/
int fingerprint_destroy(void);

/******************************************************************************
 *  Description: Reset fingerprint sensor
 *
 *  Param: NULL
 *
 *  Return: 0 success, -1 failure
 *
 *****************************************************************************/
int fingerprint_reset(void);

/*****************************************************************************
 *  Description: Enroll fingerprint
 *
 *  Param: NULL
 *
 *  Return: 0 success, -1 failure
 *
 ****************************************************************************/
int fingerprint_enroll(void);

/****************************************************************************
 *  Description: Authenticate fingerprint
 *
 *  Param: NULL
 *
 *  Return: 0 success, -1 failure
 *
 ***************************************************************************/
int fingerprint_authenticate(void);

/***************************************************************************
 *  Description: Delete fingerprint
 *
 *  Param: fingerprint_id(int), the id of fingerprint to delete (1 ~ MAX(20))
 *             type, 0 delete by index (default), 1 delete all
 *
 *  Return: 0 success, -1 failure
 *
 **************************************************************************/
int fingerprint_delete(int fingerprint_id, int type);

/**************************************************************************
 *  Description: Cancel regist/verify fingerprint
 *
 *  Param: NULL
 *
 *  Return: 0 success, -1 failure
 *
 **************************************************************************/
int fingerprint_cancel(void);

/**************************************************************************
 *  Description: Set timeout of enroll(waiting finger down/up)
 *
 *  Param: enroll_timeout(int), the timeout of enroll(waiting finger down/up)
 *
 *  Return: 0 success, -1 failure
 *
 **************************************************************************/
int fingerprint_set_enroll_timeout(int enroll_timeout);

/**************************************************************************
 *  Description: Set timeout of authenticate(waiting finger down/up)
 *
 *  Param: authenticate_timeout(int), the timeout of authenticate(waiting finger down/up)
 *
 *  Return: 0 success, -1 failure
 *
 *************************************************************************/
int fingerprint_set_authenticate_timeout(int authenticate_timeout);

/**************************************************************************
 *  Description: Get template info, including the current template sum
 *                    and the template serial index in array
 *
 *  Param:  output template_info(int []), the serial index of template (array)
 *                       template_info[MAX]
 *
 *  Return:  -1 ~ MAX(20), the template sum
 *              -1 failure
 *               >=0 success
 *
 *************************************************************************/
int fingerprint_get_template_info(int template_info[]);

/*************************************************************************
 * Description: Do software prepare after power on.
 *
 * Param: NULL
 *
 * Return: 0 success, otherwise failed.
 *
 ************************************************************************/
 int fingerprint_post_power_on(void);

/************************************************************************
 * Description: Do software prepare after power off.
 *
 * Param: NULL
 *
 * Return: 0 success, otherwise failed.
 *
 ***********************************************************************/
 int fingerprint_post_power_off(void);



#endif  // __GOODIX_INTERFACE_H__
