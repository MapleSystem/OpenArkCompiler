/*
 * Copyright (c) [2020] Huawei Technologies Co.,Ltd.All rights reserved.
 *
 * OpenArkCompiler is licensed under the Mulan PSL v1.
 * You can use this software according to the terms and conditions of the Mulan PSL v1.
 * You may obtain a copy of Mulan PSL v1 at:
 *
 *     http://license.coscl.org.cn/MulanPSL
 *
 * THIS SOFTWARE IS PROVIDED ON AN "AS IS" BASIS, WITHOUT WARRANTIES OF ANY KIND, EITHER
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO NON-INFRINGEMENT, MERCHANTABILITY OR
 * FIT FOR A PARTICULAR PURPOSE.
 * See the Mulan PSL v1 for more details.
 * -@TestCaseID: ThrowableAddSuppressedExceptionTest.java
 * -@TestCaseName: Exception in Throwable: public final void addSuppressed(Throwable exception).
 * -@TestCaseType: Function Test
 * -@RequirementName: [运行时需求]支持Java异常处理
 * -@Brief:
 * -#step1: Create throwable instance and parameter throwable instance exception.
 * -#step2: Test method addSuppressed(Throwable exception).
 * -#step3: Check that if IllegalArgumentException occurs.
 * -@Expect: 0\n
 * -@Priority: High
 * -@Source: ThrowableAddSuppressedExceptionTest.java
 * -@ExecuteClass: ThrowableAddSuppressedExceptionTest
 * -@ExecuteArgs:
 */

import java.io.PrintStream;

public class ThrowableAddSuppressedExceptionTest {
    private static int processResult = 99;

    public static void main(String[] argv) {
        System.out.println(run(argv, System.out));
    }

    /**
     * main test fun
     *
     * @return status code
     */
    public static int run(String[] argv, PrintStream out) {
        int result = 2; /* STATUS_FAILED */

        try {
            result = throwableAddSuppressedException();
        } catch (Exception e) {
            processResult -= 10;
        }

        if (result == 4 && processResult == 98) {
            result = 0;
        }
        return result;
    }

    /**
     * Exception in Throwable: public final void addSuppressed(Throwable exception).
     *
     * @return status code
     */
    public static int throwableAddSuppressedException() {
        int result1 = 4; /* STATUS_FAILED */
        Throwable cause = new Throwable();

        try {
            cause.addSuppressed(cause);
            processResult -= 10;
        } catch (IllegalArgumentException e1) {
            processResult--;
        }
        return result1;
    }
}

// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan 0\n