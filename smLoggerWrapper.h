/*
 * libsmLogger.h
 *
 *  Created on: Dec 22, 2018
 *      Author: Slleepy
 */

#ifndef SMLOGGERWRAPPER_H_
#define SMLOGGERWRAPPER_H_

extern "C"
{

void initLogger(const char* path, int log_len, int swap_len);

bool logInited();
int logRead(char* content, int len);
int logWrite(const char* content, int len);

}

#endif /* SMLOGGERWRAPPER_H_ */
