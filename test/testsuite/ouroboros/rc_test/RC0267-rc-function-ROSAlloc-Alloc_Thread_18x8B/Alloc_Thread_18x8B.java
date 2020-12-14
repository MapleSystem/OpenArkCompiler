/*
 *- @TestCaseID:Alloc_Thread_18x8B
 *- @TestCaseName:MyselfClassName
 *- @RequirementID:SR-10620538
 *- @RequirementName:[运行时需求]支持自动内存管理
 *- @Title:ROS Allocator is in charge of applying and releasing objects.This mulit thread testcase is mainly for testing objects from 8B to 18*8B(max)
 *- @Condition: no
 * -#c1
 *- @Brief:functionTest
 * -#step1
 *- @Expect:ExpectResult\n
 *- @Priority: High
 *- @Source: Alloc_Thread_18x8B.java
 *- @ExecuteClass: Alloc_Thread_18x8B
 *- @ExecuteArgs:
 *- @Remark:
 */
import java.util.ArrayList;

class Alloc_Thread_18x8B_01 extends Thread {
    private  final static int PAGE_SIZE=4*1024;
    private final static int OBJ_HEADSIZE = 8;
    private final static int MAX_18_8B=18*8;
    private static boolean checkout=false;

    public void run() {
        ArrayList<byte[]> store=new ArrayList<byte[]>();
        byte[] temp;
        for(int i=1;i<=MAX_18_8B-OBJ_HEADSIZE;i++)
        {
            for(int j=0;j<(PAGE_SIZE*64/(i+OBJ_HEADSIZE)+10);j++)
            {
                temp = new byte[i];
                store.add(temp);
            }
        }
        int check_size=store.size();
        //System.out.println(check_size);
        if(check_size == 743858)
            checkout=true;

    }
    public boolean check() {
        return checkout;
    }

}

public class Alloc_Thread_18x8B {
    public static void main(String[] args){
        Alloc_Thread_18x8B_01 test1=new Alloc_Thread_18x8B_01();
        test1.start();
        try{
            test1.join();
        }catch (InterruptedException e){}
        if(test1.check()==true)
            System.out.println("ExpectResult");
        else
            System.out.println("Error");

    }

}
// EXEC:%maple  %f %build_option -o %n.so
// EXEC:%run %n.so %n %run_option | compare %f
// ASSERT: scan-full ExpectResult\n
