#include "node_api.h"
#include <stdlib.h>
#include "libDB.h"
#include <string.h>

void catch_error_libDB(napi_env env, napi_status status) {
    if (status != napi_ok)
    {
        // napi_extended_error_info��һ���ṹ�壬����������Ϣ
        /**
         * error_message��������ı���ʾ
         * engine_reserved����͸���ֱ�����������ʹ��
         * engine_error_code��VM�ض�������
         * error_code����һ�������n-api״̬����
        */
        const napi_extended_error_info* error_info = NULL;
        // ��ȡ�쳣��Ϣ
        napi_get_last_error_info(env, &error_info);
        // ����ʹ��napi_value��ʾ�Ĵ�����Ϣ
        napi_value error_msg;
        napi_create_string_utf8(
            env,
            error_info->error_message,
            NAPI_AUTO_LENGTH,
            &error_msg
        );
        // �׳�������������JS����һ��uncaughtException�쳣
        napi_fatal_exception(env, error_msg);
        // �˳�����ִ��
        exit(0);
    }
}
/***************************getHWVersion**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_hw_version;


void work_complete_get_hw_version(napi_env env, napi_status status, void* data) {
    AddonData_get_hw_version* addon_data = (AddonData_get_hw_version*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_hw_version(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_hw_version_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* version = getHWVersion();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, version, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_hw_version(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_hw_version* addon_data = (AddonData_get_hw_version*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* version = getHWVersion();
    char* v = (char*)malloc(sizeof(char) * (strlen(version) + 1));
    strcpy(v, version);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        v,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_hw_version(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_hw_version* addon_data = (AddonData_get_hw_version*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_hw_version,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_hw_version,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_hw_version,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************getNCVersion**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_nc_version;


void work_complete_get_nc_version(napi_env env, napi_status status, void* data) {
    AddonData_get_nc_version* addon_data = (AddonData_get_nc_version*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_nc_version(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_nc_version_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* version = getNCVersion();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, version, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_nc_version(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_nc_version* addon_data = (AddonData_get_nc_version*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* version = getNCVersion();
    char* v = (char*)malloc(sizeof(char) * (strlen(version) + 1));
    strcpy(v, version);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        v,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_nc_version(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_nc_version* addon_data = (AddonData_get_nc_version*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_nc_version,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_nc_version,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_nc_version,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************get_fota_setting**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_fota_setting;


void work_complete_get_fota_setting(napi_env env, napi_status status, void* data) {
    AddonData_get_fota_setting* addon_data = (AddonData_get_fota_setting*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_fota_setting(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_fota_setting_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* result = getFOTASetting();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_fota_setting(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_fota_setting* addon_data = (AddonData_get_fota_setting*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getFOTASetting();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_fota_setting(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_fota_setting* addon_data = (AddonData_get_fota_setting*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_fota_setting,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_fota_setting,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_fota_setting,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************get_firmware_version**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_firmware_version;


void work_complete_get_firmware_version(napi_env env, napi_status status, void* data) {
    AddonData_get_firmware_version* addon_data = (AddonData_get_firmware_version*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_firmware_version(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_firmware_version_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* result = getFirmwareVersion();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_firmware_version(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_firmware_version* addon_data = (AddonData_get_firmware_version*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getFirmwareVersion();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_firmware_version(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_firmware_version* addon_data = (AddonData_get_firmware_version*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_firmware_version,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_firmware_version,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_firmware_version,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/***************************getFirmwareVersionFull**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_firmware_version_full;


void work_complete_get_firmware_version_full(napi_env env, napi_status status, void* data) {
    AddonData_get_firmware_version_full* addon_data = (AddonData_get_firmware_version_full*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_firmware_version_full(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_firmware_version_full_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* result = getFirmwareVersionFull();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_firmware_version_full(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_firmware_version_full* addon_data = (AddonData_get_firmware_version_full*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getFirmwareVersionFull();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_firmware_version_full(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_firmware_version_full* addon_data = (AddonData_get_firmware_version_full*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_firmware_version_full,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_firmware_version_full,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_firmware_version_full,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/***************************get_firmware_info**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_firmware_info;


void work_complete_get_firmware_info(napi_env env, napi_status status, void* data) {
    AddonData_get_firmware_info* addon_data = (AddonData_get_firmware_info*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_firmware_info(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_firmware_info_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* result = getFirmwareInfo();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_firmware_info(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_firmware_info* addon_data = (AddonData_get_firmware_info*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getFirmwareInfo();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_firmware_info(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_firmware_info* addon_data = (AddonData_get_firmware_info*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_firmware_info,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_firmware_info,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_firmware_info,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************getDeviceUUID**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_device_uuid;


void work_complete_get_device_uuid(napi_env env, napi_status status, void* data) {
    AddonData_get_device_uuid* addon_data = (AddonData_get_device_uuid*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_device_uuid(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value get_device_uuid_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    char* result = getDeviceUUID();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_device_uuid(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_device_uuid* addon_data = (AddonData_get_device_uuid*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getDeviceUUID();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_device_uuid(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_device_uuid* addon_data = (AddonData_get_device_uuid*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_device_uuid,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_device_uuid,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_device_uuid,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************get_firmware_upgrade_status**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_firmware_upgrade_status;


void work_complete_get_firmware_upgrade_status(napi_env env, napi_status status, void* data) {
    AddonData_get_firmware_upgrade_status* addon_data = (AddonData_get_firmware_upgrade_status*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_firmware_upgrade_status(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}

 napi_value get_firmware_upgrade_status_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    int result = getFirmwareUpgradeStatus();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_firmware_upgrade_status(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_firmware_upgrade_status* addon_data = (AddonData_get_firmware_upgrade_status*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = getFirmwareUpgradeStatus();
    //char* rs = (char*)malloc(sizeof(strlen(result)));
    //strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_firmware_upgrade_status(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_firmware_upgrade_status* addon_data = (AddonData_get_firmware_upgrade_status*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_firmware_upgrade_status,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_firmware_upgrade_status,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_firmware_upgrade_status,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************get_fota_update_status**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
} AddonData_get_fota_update_status;


void work_complete_get_fota_update_status(napi_env env, napi_status status, void* data) {
    AddonData_get_fota_update_status* addon_data = (AddonData_get_fota_update_status*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_get_fota_update_status(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}

 napi_value get_fota_update_status_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //ִ��so����
    int result = getFOTAUpdateStatus();
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_fota_update_status(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_fota_update_status* addon_data = (AddonData_get_fota_update_status*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = getFOTAUpdateStatus();
    //char* rs = (char*)malloc(sizeof(strlen(result)));
    //strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_fota_update_status(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 1)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_fota_update_status* addon_data = (AddonData_get_fota_update_status*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_fota_update_status,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_fota_update_status,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_fota_update_status,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************setFOTASetting**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
    int nEnable;
    int nWeekday;
    int nHour;
    int nMinute;
    int nUpdateBetaFw;
} AddonData_set_fota_setting;


void work_complete_set_fota_setting(napi_env env, napi_status status, void* data) {
    AddonData_set_fota_setting* addon_data = (AddonData_set_fota_setting*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_set_fota_setting(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}

 napi_value set_fota_setting_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 6;
    napi_value argv[6];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 6)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //nEnable
    int nEnable;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nEnable));
    //nWeekday
    int nWeekday;
    catch_error_libDB(env, napi_get_value_int32(env, argv[1], &nWeekday));
    //nHour
    int nHour;
    catch_error_libDB(env, napi_get_value_int32(env, argv[2], &nHour));
    //nMinute
    int nMinute;
    catch_error_libDB(env, napi_get_value_int32(env, argv[3], &nMinute));
    //nUpdateBetaFw
    int nUpdateBetaFw;
    catch_error_libDB(env, napi_get_value_int32(env, argv[4], &nUpdateBetaFw));
    //ִ��so����
    int result = setFOTASetting(nEnable, nWeekday, nHour, nMinute, nUpdateBetaFw);
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[5], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_set_fota_setting(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_fota_setting* addon_data = (AddonData_set_fota_setting*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setFOTASetting(addon_data->nEnable, addon_data->nWeekday, addon_data->nHour, addon_data->nMinute, addon_data->nUpdateBetaFw);
    //char* rs = (char*)malloc(sizeof(strlen(result)));
    //strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_fota_setting(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 6;
    napi_value argv[6];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 6)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //nEnable
    int nEnable;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nEnable));
    //nWeekday
    int nWeekday;
    catch_error_libDB(env, napi_get_value_int32(env, argv[1], &nWeekday));
    //nHour
    int nHour;
    catch_error_libDB(env, napi_get_value_int32(env, argv[2], &nHour));
    //nMinute
    int nMinute;
    catch_error_libDB(env, napi_get_value_int32(env, argv[3], &nMinute));
    //nUpdateBetaFw
    int nUpdateBetaFw;
    catch_error_libDB(env, napi_get_value_int32(env, argv[4], &nUpdateBetaFw));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_fota_setting* addon_data = (AddonData_set_fota_setting*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    addon_data->nEnable = nEnable;
    addon_data->nWeekday = nWeekday;
    addon_data->nHour = nHour;
    addon_data->nMinute = nMinute;
    addon_data->nUpdateBetaFw = nUpdateBetaFw;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[5],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_fota_setting,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_fota_setting,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_fota_setting,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************set_fota_update_status**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
    int nStatus;
} AddonData_set_fota_update_status;


void work_complete_set_fota_update_status(napi_env env, napi_status status, void* data) {
    AddonData_set_fota_update_status* addon_data = (AddonData_set_fota_update_status*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_set_fota_update_status(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}

 napi_value set_fota_update_status_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 2)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //nStatus
    int nStatus;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nStatus));
    //ִ��so����
    int result = setFOTAUpdateStatus(nStatus);
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[1], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_set_fota_update_status(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_fota_update_status* addon_data = (AddonData_set_fota_update_status*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setFOTAUpdateStatus(addon_data->nStatus);
    //char* rs = (char*)malloc(sizeof(strlen(result)));
    //strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_fota_update_status(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 2)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //nStatus
    int nStatus;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nStatus));
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_fota_update_status* addon_data = (AddonData_set_fota_update_status*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    addon_data->nStatus = nStatus;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[1],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_fota_update_status,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_fota_update_status,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_fota_update_status,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************set_firmware_upgrade_status**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
    int nStatus;
} AddonData_set_firmware_upgrade_status;


void work_complete_set_firmware_upgrade_status(napi_env env, napi_status status, void* data) {
    AddonData_set_firmware_upgrade_status* addon_data = (AddonData_set_firmware_upgrade_status*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    //free(addon_data->libPath);
    free(addon_data);
}

void call_js_callback_set_firmware_upgrade_status(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}

 napi_value set_firmware_upgrade_status_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 2)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //nStatus
    int nStatus;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nStatus));
    //ִ��so����
    int result = setFirmwareUpgradeStatus(nStatus);
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[1], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_set_firmware_upgrade_status(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_firmware_upgrade_status* addon_data = (AddonData_set_firmware_upgrade_status*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setFirmwareUpgradeStatus(addon_data->nStatus);
    //char* rs = (char*)malloc(sizeof(strlen(result)));
    //strcpy(rs, result);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_firmware_upgrade_status(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 2)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //nStatus
    int nStatus;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nStatus));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_firmware_upgrade_status* addon_data = (AddonData_set_firmware_upgrade_status*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    addon_data->nStatus = nStatus;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[1],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_firmware_upgrade_status,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_firmware_upgrade_status,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_firmware_upgrade_status,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/***************************analyze_firmware_info**************************/
typedef struct
{
    /**
     * napi_async_work��һ���ṹ�壬���ڹ����첽�����߳�
     * ͨ��napi_create_async_work����ʵ��
     * ͨ��napi_delete_async_workɾ��ʵ��
    */
    napi_async_work work;             // �����̵߳�����
    /**
     * napi_threadsafe_function��һ��ָ��
     * ��ʾ�ڶ��߳��п���ͨ��napi_call_threadsafe_function�첽���õ�JS����
    */
    napi_threadsafe_function tsfn;    // ����ص�����
    int nMajor;
    int nMinor;
    int nRev;
    char* strFirmwareInfoFromFOTA;
    int nUpdateBetaFw;
} AddonData_analyze_firmware_info;


void work_complete_analyze_firmware_info(napi_env env, napi_status status, void* data) {
    AddonData_analyze_firmware_info* addon_data = (AddonData_analyze_firmware_info*)data;

    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_libDB(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->strFirmwareInfoFromFOTA);
    free(addon_data);
}

void call_js_callback_analyze_firmware_info(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_libDB(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_libDB(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_libDB(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
    if (data != NULL)
    {
        // �ͷ�����
        free((char*)data);
    }
}

 napi_value analyze_firmware_info_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 6;
    napi_value argv[6];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 6)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // ��һ����������̬���ӿ��ȫ·��(int nMajor, int nMinor, int nRev, char* strFirmwareInfoFromFOTA, int nUpdateBetaFw)
    size_t strLength;
    //nMajor
    int nMajor;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nMajor));
    //nMinor
    int nMinor;
    catch_error_libDB(env, napi_get_value_int32(env, argv[1], &nMinor));
    //nRev
    int nRev;
    catch_error_libDB(env, napi_get_value_int32(env, argv[2], &nRev));
    //strFirmwareInfoFromFOTA
    catch_error_libDB(env, napi_get_value_string_utf8(env, argv[3], NULL, NULL, &strLength));
    char strFirmwareInfoFromFOTA[++strLength];
    catch_error_libDB(env, napi_get_value_string_utf8(env, argv[3], strFirmwareInfoFromFOTA, strLength, &strLength));
    //nUpdateBetaFw
    int nUpdateBetaFw;
    catch_error_libDB(env, napi_get_value_int32(env, argv[4], &nUpdateBetaFw));
    //ִ��so����
    char* info = analyzeFirmwareInfo(nMajor, nMinor, nRev, strFirmwareInfoFromFOTA, nUpdateBetaFw);
    //ת����
    napi_value world;
    catch_error_libDB(env, napi_create_string_utf8(env, info, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_value* callbackParams = &world;
    size_t callbackArgc = 1;
    napi_value global;
    napi_get_global(env, &global);
    napi_value callbackRs;
    napi_call_function(env, global, argv[5], callbackArgc, callbackParams, &callbackRs);
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_analyze_firmware_info(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_analyze_firmware_info* addon_data = (AddonData_analyze_firmware_info*)data;
    //��ȡjs-callback����
    catch_error_libDB(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* info = analyzeFirmwareInfo(addon_data->nMajor, addon_data->nMinor, addon_data->nRev, addon_data->strFirmwareInfoFromFOTA, addon_data->nUpdateBetaFw);
    char* v = (char*)malloc(sizeof(char) * (strlen(info) + 1));
    strcpy(v, info);
    // ����js-callback����
    catch_error_libDB(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        v,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_libDB(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value analyze_firmware_info(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 6;
    napi_value argv[6];
    catch_error_libDB(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 6)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    // �����߳�����
    catch_error_libDB(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // ��һ����������̬���ӿ��ȫ·��(int nMajor, int nMinor, int nRev, char* strFirmwareInfoFromFOTA, int nUpdateBetaFw)
    size_t strLength;
    //nMajor
    int nMajor;
    catch_error_libDB(env, napi_get_value_int32(env, argv[0], &nMajor));
    //nMinor
    int nMinor;
    catch_error_libDB(env, napi_get_value_int32(env, argv[1], &nMinor));
    //nRev
    int nRev;
    catch_error_libDB(env, napi_get_value_int32(env, argv[2], &nRev));
    //strFirmwareInfoFromFOTA
    catch_error_libDB(env, napi_get_value_string_utf8(env, argv[3], NULL, NULL, &strLength));
    char strFirmwareInfoFromFOTA[++strLength];
    catch_error_libDB(env, napi_get_value_string_utf8(env, argv[3], strFirmwareInfoFromFOTA, strLength, &strLength));
    //nUpdateBetaFw
    int nUpdateBetaFw;
    catch_error_libDB(env, napi_get_value_int32(env, argv[4], &nUpdateBetaFw));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_analyze_firmware_info* addon_data = (AddonData_analyze_firmware_info*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    addon_data->work = NULL;
    addon_data->nMajor = nMajor;
    addon_data->nMinor = nMinor;
    addon_data->nRev = nRev;
    addon_data->strFirmwareInfoFromFOTA = (char*)malloc(sizeof(char) * (strlen(strFirmwareInfoFromFOTA) + 1));
    strcpy(addon_data->strFirmwareInfoFromFOTA, strFirmwareInfoFromFOTA);
    addon_data->strFirmwareInfoFromFOTA = NULL;
    addon_data->nUpdateBetaFw = nUpdateBetaFw;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_libDB(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_analyze_firmware_info,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_libDB(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_analyze_firmware_info,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_analyze_firmware_info,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_libDB(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/**************************************init**************************************/
napi_value init(napi_env env, napi_value exports)
{
    /***************************getHWVersion**************************/
    // ��ȡ��������
    napi_property_descriptor getHWVersionSync = {
        "getHWVersionSync",
        NULL,
        get_hw_version_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getHWVersionSync
    );

    // ��ȡ��������
    napi_property_descriptor getHWVersion = {
        "getHWVersion",
        NULL,
        get_hw_version,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getHWVersion
    );
    /***************************getNCVersion**************************/
    // ��ȡ��������
    napi_property_descriptor getNCVersionSync = {
        "getNCVersionSync",
        NULL,
        get_nc_version_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getNCVersionSync
    );

    // ��ȡ��������
    napi_property_descriptor getNCVersion = {
        "getNCVersion",
        NULL,
        get_nc_version,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getNCVersion
    );
    /***************************getFOTASetting**************************/
    // ��ȡ��������
    napi_property_descriptor getFOTASettingSync = {
        "getFOTASettingSync",
        NULL,
        get_fota_setting_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFOTASettingSync
    );

    // ��ȡ��������
    napi_property_descriptor getFOTASetting = {
        "getFOTASetting",
        NULL,
        get_fota_setting,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFOTASetting
    );
    /***************************getFirmwareVersion**************************/
    // ��ȡ��������
    napi_property_descriptor getFirmwareVersionSync = {
        "getFirmwareVersionSync",
        NULL,
        get_firmware_version_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareVersionSync
    );

    // ��ȡ��������
    napi_property_descriptor getFirmwareVersion = {
        "getFirmwareVersion",
        NULL,
        get_firmware_version,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareVersion
    );
    /***************************getFirmwareVersionFull**************************/
    // ��ȡ��������
    napi_property_descriptor getFirmwareVersionFullSync = {
        "getFirmwareVersionFullSync",
        NULL,
        get_firmware_version_full_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareVersionFullSync
    );

    // ��ȡ��������
    napi_property_descriptor getFirmwareVersionFull = {
        "getFirmwareVersionFull",
        NULL,
        get_firmware_version_full,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareVersionFull
    );
    /***************************getFirmwareInfo**************************/
// ��ȡ��������
    napi_property_descriptor getFirmwareInfoSync = {
        "getFirmwareInfoSync",
        NULL,
        get_firmware_info_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareInfoSync
    );

    // ��ȡ��������
    napi_property_descriptor getFirmwareInfo = {
        "getFirmwareInfo",
        NULL,
        get_firmware_info,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareInfo
    );
    /***************************getDeviceUUID**************************/
// ��ȡ��������
    napi_property_descriptor getDeviceUUIDSync = {
        "getDeviceUUIDSync",
        NULL,
        get_device_uuid_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getDeviceUUIDSync
    );

    // ��ȡ��������
    napi_property_descriptor getDeviceUUID = {
        "getDeviceUUID",
        NULL,
        get_device_uuid,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getDeviceUUID
    );
    /***************************getFirmwareUpgradeStatus**************************/
// ��ȡ��������
    napi_property_descriptor getFirmwareUpgradeStatusSync = {
        "getFirmwareUpgradeStatusSync",
        NULL,
        get_firmware_upgrade_status_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareUpgradeStatusSync
    );

    // ��ȡ��������
    napi_property_descriptor getFirmwareUpgradeStatus = {
        "getFirmwareUpgradeStatus",
        NULL,
        get_firmware_upgrade_status,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFirmwareUpgradeStatus
    );
    /***************************getFOTAUpdateStatus**************************/
    // ��ȡ��������
    napi_property_descriptor getFOTAUpdateStatusSync = {
        "getFOTAUpdateStatusSync",
        NULL,
        get_fota_update_status_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFOTAUpdateStatusSync
    );

    // ��ȡ��������
    napi_property_descriptor getFOTAUpdateStatus = {
        "getFOTAUpdateStatus",
        NULL,
        get_fota_update_status,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &getFOTAUpdateStatus
    );
    /***************************setFOTASetting**************************/
    // ��ȡ��������
    napi_property_descriptor setFOTASettingSync = {
        "setFOTASettingSync",
        NULL,
        set_fota_setting_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFOTASettingSync
    );

    // ��ȡ��������
    napi_property_descriptor setFOTASetting = {
        "setFOTASetting",
        NULL,
        set_fota_setting,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFOTASetting
    );
    /***************************setFOTAUpdateStatus**************************/
// ��ȡ��������
    napi_property_descriptor setFOTAUpdateStatusSync = {
        "setFOTAUpdateStatusSync",
        NULL,
        set_fota_update_status_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFOTAUpdateStatusSync
    );

    // ��ȡ��������
    napi_property_descriptor setFOTAUpdateStatus = {
        "setFOTAUpdateStatus",
        NULL,
        set_fota_update_status,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFOTAUpdateStatus
    );
    /***************************setFirmwareUpgradeStatus**************************/
// ��ȡ��������
    napi_property_descriptor setFirmwareUpgradeStatusSync = {
        "setFirmwareUpgradeStatusSync",
        NULL,
        set_firmware_upgrade_status_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFirmwareUpgradeStatusSync
    );

    // ��ȡ��������
    napi_property_descriptor setFirmwareUpgradeStatus = {
        "setFirmwareUpgradeStatus",
        NULL,
        set_firmware_upgrade_status,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &setFirmwareUpgradeStatus
    );
    /***************************analyzeFirmwareInfo**************************/
// ��ȡ��������
    napi_property_descriptor analyzeFirmwareInfoSync = {
        "analyzeFirmwareInfoSync",
        NULL,
        analyze_firmware_info_sync,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &analyzeFirmwareInfoSync
    );

    // ��ȡ��������
    napi_property_descriptor analyzeFirmwareInfo = {
        "analyzeFirmwareInfo",
        NULL,
        analyze_firmware_info,
        NULL,
        NULL,
        NULL,
        napi_default,
        NULL
    };
    //��¶�ӿ�
    napi_define_properties(
        env,
        exports,
        1,
        &analyzeFirmwareInfo
    );
    return exports;
}

NAPI_MODULE(NODE_GYP_MODULE_NAME, init);