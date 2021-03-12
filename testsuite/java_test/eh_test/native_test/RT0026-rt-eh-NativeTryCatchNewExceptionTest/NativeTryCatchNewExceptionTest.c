/*
 * Copyright (c) [2021] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under Mulan PSL v2.
 * You can use this software according to the terms and conditions of the Mulan PSL v2.
 *
 *     http://license.coscl.org.cn/MulanPSL2
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v2 for more details.
 */

#include <stdio.h>
#include <jni.h>
#include <stdlib.h>
#include <assert.h>

void native_nativeNativeTryCatchNewExceptionTest (JNIEnv *env, jobject obj){
    jclass cls = (*env)->GetObjectClass(env, obj);
    jmethodID mid = (*env)->GetMethodID(env, cls, "callback", "()V");
    if (mid == NULL){
        return;
    }
    (*env)->CallVoidMethod(env, obj, mid);
    jclass newExcCls = (*env)->FindClass(env,"java/lang/StringIndexOutOfBoundsException");
    (*env)->ThrowNew(env,newExcCls,"NativeThrowNew");

    printf("------>CheckPoint:CcanContinue\n");
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM* vm, void* reserved)
{
    JNIEnv* env = NULL;
    jint result = -1;

    if ((*vm)->GetEnv(vm, (void**) &env, JNI_VERSION_1_4) != JNI_OK) {
        return -1;
    }
    assert(env != NULL);

        JNINativeMethod gMethods [] =
        {
                {"nativeNativeTryCatchNewExceptionTest", "()V", (void*)&native_nativeNativeTryCatchNewExceptionTest },
        };
        // if package is test:"test/NativeTryCatchNewExceptionTest"
        jclass cls = (*env)->FindClass(env, "NativeTryCatchNewExceptionTest");
        (*env)->RegisterNatives(env, cls, gMethods, 1);

    /* success -- return valid version number */
    result = JNI_VERSION_1_4;

    return result;
}

