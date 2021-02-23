#include <node_api.h>
#include <stdlib.h>
#include <string.h>
#include "libDatetimeSetting.h"


void catch_error_get_datetime(napi_env env, napi_status status) {
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
/****************************get_datetime***************************************/
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
} AddonData_get_datetime;

void work_complete_get_datetime(napi_env env, napi_status status, void* data) {
    AddonData_get_datetime* addon_data = (AddonData_get_datetime*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data);
}

void call_js_callback_get_datetime(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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

 napi_value get_datetime_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    char* time = getDatetime();
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_string_utf8(env, time, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    }
    return world;
}

/**
 * ִ���߳�
*/
 void execute_work_get_datetime(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_datetime* addon_data = (AddonData_get_datetime*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* time = getDatetime();
    char* uptime = (char*)malloc(sizeof(char) * (strlen(time) + 1));
    strcpy(uptime, time);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        uptime,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_datetime(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_datetime* addon_data = (AddonData_get_datetime*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_datetime,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_datetime,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_datetime,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************get_timezone***************************************/
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
} AddonData_get_timezone;

 napi_value get_timezone_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    char* time = getTimezone();
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_string_utf8(env, time, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    }
    return world;
}

void work_complete_get_timezone(napi_env env, napi_status status, void* data) {
    AddonData_get_timezone* addon_data = (AddonData_get_timezone*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data);
}

void call_js_callback_get_timezone(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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
/**
 * ִ���߳�
*/
 void execute_work_get_timezone(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_timezone* addon_data = (AddonData_get_timezone*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* time = getTimezone();
    char* uptime = (char*)malloc(sizeof(char) * (strlen(time) +1));
    strcpy(uptime, time);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        uptime,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_timezone(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_timezone* addon_data = (AddonData_get_timezone*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_timezone,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_timezone,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_timezone,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************get_daylight_saving***************************************/
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
} AddonData_get_daylight_saving;

 napi_value get_daylight_saving_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    char* result = getDaylightSaving();
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    }
    return world;
}

void work_complete_get_daylight_saving(napi_env env, napi_status status, void* data) {
    AddonData_get_daylight_saving* addon_data = (AddonData_get_daylight_saving*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data);
}

void call_js_callback_get_daylight_saving(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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
/**
 * ִ���߳�
*/
 void execute_work_get_daylight_saving(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_daylight_saving* addon_data = (AddonData_get_daylight_saving*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getDaylightSaving();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_daylight_saving(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_daylight_saving* addon_data = (AddonData_get_daylight_saving*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_daylight_saving,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_daylight_saving,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_daylight_saving,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************get_city_datetime***************************************/
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
} AddonData_get_city_datetime;

 napi_value get_city_datetime_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    char* result = getCityDatetime();
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[0], callbackArgc, callbackParams, &callbackRs);
    }
    return world;
}

void work_complete_get_city_datetime(napi_env env, napi_status status, void* data) {
    AddonData_get_city_datetime* addon_data = (AddonData_get_city_datetime*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data);
}

void call_js_callback_get_city_datetime(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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
/**
 * ִ���߳�
*/
 void execute_work_get_city_datetime(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_city_datetime* addon_data = (AddonData_get_city_datetime*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getCityDatetime();
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_city_datetime(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 1;
    napi_value argv[1];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[0], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_get_city_datetime* addon_data = (AddonData_get_city_datetime*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[0],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_city_datetime,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_city_datetime,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_city_datetime,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}

/****************************setDatetime***************************************/
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
    int isPermanent;
    int enableNTPServer;
    char* strServerAddress;
    char* strDatetime;
} AddonData_set_datetime;

 napi_value set_datetime_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 5;
    napi_value argv[5];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 5)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //��ȡ����(int isPermanent, int enableNTPServer, char* strServerAddress, char* strDatetime)
    size_t strLength;
    //isPermanent
    int isPermanent;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &isPermanent));
    //enableNTPServer
    int enableNTPServer;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &enableNTPServer));
    //strServerAddress
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[2], NULL, NULL, &strLength));
    char* strServerAddress = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[2], strServerAddress, strLength, &strLength));
    //strDatetime
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[3], NULL, NULL, &strLength));
    char* strDatetime = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[3], strDatetime, strLength, &strLength));
    //ִ��so����
    int result = setDatetime(isPermanent, enableNTPServer, strServerAddress, strDatetime);
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[4], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[4], callbackArgc, callbackParams, &callbackRs);
    }
    free(strServerAddress);
    free(strDatetime);
    return world;
}

void work_complete_set_datetime(napi_env env, napi_status status, void* data) {
    AddonData_set_datetime* addon_data = (AddonData_set_datetime*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->strServerAddress);
    free(addon_data->strDatetime);
    free(addon_data);
}

void call_js_callback_set_datetime(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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
/**
 * ִ���߳�
*/
 void execute_work_set_datetime(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_datetime* addon_data = (AddonData_set_datetime*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setDatetime(addon_data->isPermanent, addon_data->enableNTPServer, addon_data->strServerAddress, addon_data->strDatetime);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_datetime(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 5;
    napi_value argv[5];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 5)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[4], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //��ȡ����(int isPermanent, int enableNTPServer, char* strServerAddress, char* strDatetime)
    size_t strLength;
    AddonData_set_datetime* addon_data = (AddonData_set_datetime*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &addon_data->isPermanent));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &addon_data->enableNTPServer));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[2], NULL, NULL, &strLength));
    addon_data->strServerAddress = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[2], addon_data->strServerAddress, strLength, &strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[3], NULL, NULL, &strLength));
    addon_data->strDatetime = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[3], addon_data->strDatetime, strLength, &strLength));
    //strServerAddress
    //strDatetime
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[4],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_datetime,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_datetime,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_datetime,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************set_timezone***************************************/
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
    int isPermanent;
    char* strTimezone;
} AddonData_set_timezone;

 napi_value set_timezone_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 3;
    napi_value argv[3];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 3)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //��ȡ����(int isPermanent, int enableNTPServer, char* strServerAddress, char* strDatetime)
    size_t strLength;
    //isPermanent
    int isPermanent;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &isPermanent));
    //strTimezone
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[1], NULL, NULL, &strLength));
    char* strTimezone = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[1], strTimezone, strLength, &strLength));
    //ִ��so����
    int result = setTimezone(isPermanent, strTimezone);
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[2], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[2], callbackArgc, callbackParams, &callbackRs);
    }
    free(strTimezone);
    return world;
}

void work_complete_set_timezone(napi_env env, napi_status status, void* data) {
    AddonData_set_timezone* addon_data = (AddonData_set_timezone*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->strTimezone);
    free(addon_data);
}

void call_js_callback_set_timezone(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}
/**
 * ִ���߳�
*/
 void execute_work_set_timezone(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_timezone* addon_data = (AddonData_set_timezone*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setTimezone(addon_data->isPermanent, addon_data->strTimezone);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_timezone(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 3;
    napi_value argv[3];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 3)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[2], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //��ȡ����(int isPermanent, int enableNTPServer, char* strServerAddress, char* strDatetime)
    size_t strLength;
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_timezone* addon_data = (AddonData_set_timezone*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &addon_data->isPermanent));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[1], NULL, NULL, &strLength));
    addon_data->strTimezone = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[1], addon_data->strTimezone, strLength, &strLength));
    //isPermanent
    int isPermanent;
    //strTimezone

    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[2],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_timezone,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_timezone,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_timezone,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************set_daylight_saving***************************************/
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
    int isdst;
    int offsetSeconds;
    int startMonth;
    int startWeek;
    int startDay;
    char* startTime;
    int endMonth;
    int endWeek;
    int endDay;
    char* endTime;
} AddonData_set_daylight_saving;

 napi_value set_daylight_saving_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 11;
    napi_value argv[11];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 11)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //��ȡ����(int isdst, int offsetSeconds, int startMonth, int startWeek, int startDay, char* startTime,
    //int endMonth, int endWeek, int endDay, char* endTime)
    size_t strLength;
    //isdst
    int isdst;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &isdst));
    //offsetSeconds
    int offsetSeconds;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &offsetSeconds));
    //startMonth
    int startMonth;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[2], &startMonth));
    //startWeek
    int startWeek;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[3], &startWeek));
    //startDay
    int startDay;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[4], &startDay));
    //strTimezone
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[5], NULL, NULL, &strLength));
    char* startTime = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[5], startTime, strLength, &strLength));
    //endMonth
    int endMonth;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[6], &endMonth));
    //endWeek
    int endWeek;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[7], &endWeek));
    //endDay
    int endDay;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[8], &endDay));
    //endTime
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[9], NULL, NULL, &strLength));
    char* endTime = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[9], endTime, strLength, &strLength));
    //ִ��so����
    int result = setDaylightSaving(isdst,offsetSeconds,startMonth, startWeek, startDay, startTime,endMonth, endWeek, endDay, endTime);
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[10], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[10], callbackArgc, callbackParams, &callbackRs);
    }
    free(startTime);
    free(endTime);
    return world;
}

void work_complete_set_daylight_saving(napi_env env, napi_status status, void* data) {
    AddonData_set_daylight_saving* addon_data = (AddonData_set_daylight_saving*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->startTime);
    free(addon_data->endTime);
    free(addon_data);
}

void call_js_callback_set_daylight_saving(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}
/**
 * ִ���߳�
*/
 void execute_work_set_daylight_saving(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_daylight_saving* addon_data = (AddonData_set_daylight_saving*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setDaylightSaving(addon_data->isdst, addon_data->offsetSeconds, addon_data->startMonth, addon_data->startWeek, addon_data->startDay, addon_data->startTime,
        addon_data->endMonth, addon_data->endWeek, addon_data->endDay, addon_data->endTime);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_daylight_saving(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 11;
    napi_value argv[11];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 11)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[10], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //��ȡ����(int isdst, int offsetSeconds, int startMonth, int startWeek, int startDay, char* startTime,
//int endMonth, int endWeek, int endDay, char* endTime)
    size_t strLength;
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_daylight_saving* addon_data = (AddonData_set_daylight_saving*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[0], &addon_data->isdst));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &addon_data->offsetSeconds));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[2], &addon_data->startMonth));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[3], &addon_data->startWeek));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[4], &addon_data->startDay));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[5], NULL, NULL, &strLength));
    addon_data->startTime = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[5], addon_data->startTime, strLength, &strLength));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[6], &addon_data->endMonth));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[7], &addon_data->endWeek));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[8], &addon_data->endDay));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[9], NULL, NULL, &strLength));
    addon_data->endTime = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[9], addon_data->endTime, strLength, &strLength));

    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[10],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_daylight_saving,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_daylight_saving,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_daylight_saving,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/****************************set_city_datetime***************************************/
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
    char* strCityId; 
    int isDstEnabled;
    int offsetSeconds;
    int startMonth;
    int startWeek;
    int startDay;
    char* startTime;
    int endMonth;
    int endWeek;
    int endDay;
    char* endTime;
} AddonData_set_city_datetime;

 napi_value set_city_datetime_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 12;
    napi_value argv[12];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 12)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //��ȡ����(char* strCityId, int isDstEnabled, int offsetSeconds, int startMonth, int startWeek, int startDay, char* startTime,
    //int endMonth, int endWeek, int endDay, char* endTime)
    size_t strLength;
    //strCityId
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], NULL, NULL, &strLength));
    char* strCityId = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], strCityId, strLength, &strLength));
    //isDstEnabled
    int isDstEnabled;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &isDstEnabled));
    //offsetSeconds
    int offsetSeconds;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[2], &offsetSeconds));
    //startMonth
    int startMonth;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[3], &startMonth));
    //startWeek
    int startWeek;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[4], &startWeek));
    //startDay
    int startDay;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[5], &startDay));
    //strTimezone
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[6], NULL, NULL, &strLength));
    char* startTime = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[6], startTime, strLength, &strLength));
    //endMonth
    int endMonth;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[7], &endMonth));
    //endWeek
    int endWeek;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[8], &endWeek));
    //endDay
    int endDay;
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[9], &endDay));
    //endTime
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[10], NULL, NULL, &strLength));
    char* endTime = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[10], endTime, strLength, &strLength));
    //ִ��so����
    int result = setCityDatetime(strCityId, isDstEnabled, offsetSeconds, startMonth, startWeek, startDay, startTime, endMonth, endWeek, endDay, endTime);
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_int32(env, result, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[11], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[11], callbackArgc, callbackParams, &callbackRs);
    }
    free(strCityId);
    free(startTime);
    free(endTime);
    return world;
}

void work_complete_set_city_datetime(napi_env env, napi_status status, void* data) {
    AddonData_set_city_datetime* addon_data = (AddonData_set_city_datetime*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->strCityId);
    free(addon_data->startTime);
    free(addon_data->endTime);
    free(addon_data);
}

void call_js_callback_set_city_datetime(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_int32(
            env,
            data,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
            env,
            undefined,   // js�ص���this����
            js_cb,       // js�ص��������
            1,           // js�ص��������ܲ�������
            &callBackInParams,        // js�ص�������������
            NULL         // js�ص������������return�����ᱻresult���ܵ�
        ));
    }
}
/**
 * ִ���߳�
*/
 void execute_work_set_city_datetime(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_set_city_datetime* addon_data = (AddonData_set_city_datetime*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    int result = setCityDatetime(addon_data->strCityId,addon_data->isDstEnabled, addon_data->offsetSeconds, addon_data->startMonth, addon_data->startWeek, addon_data->startDay, addon_data->startTime,
        addon_data->endMonth, addon_data->endWeek, addon_data->endDay, addon_data->endTime);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        result,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value set_city_datetime(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 12;
    napi_value argv[12];
    catch_error_get_datetime(env, napi_get_cb_info(
        env,
        info,
        &argc,
        argv,
        NULL,
        NULL
    ));
    if (argc != 12)
    {
        napi_throw_error(env, "EINVAL", "Argument count mismatch");
    }
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[11], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //��ȡ����(char* strCityId, int isDstEnabled, int offsetSeconds, int startMonth, int startWeek, int startDay, char* startTime,
     //int endMonth, int endWeek, int endDay, char* endTime)
    size_t strLength;
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    AddonData_set_city_datetime* addon_data = (AddonData_set_city_datetime*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], NULL, NULL, &strLength));
    addon_data->strCityId = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], addon_data->strCityId, strLength, &strLength));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[1], &addon_data->isDstEnabled));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[2], &addon_data->offsetSeconds));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[3], &addon_data->startMonth));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[4], &addon_data->startWeek));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[5], &addon_data->startDay));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[6], NULL, NULL, &strLength));
    addon_data->startTime = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[6], addon_data->startTime, strLength, &strLength));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[7], &addon_data->endMonth));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[8], &addon_data->endWeek));
    catch_error_get_datetime(env, napi_get_value_int32(env, argv[9], &addon_data->endDay));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[10], NULL, NULL, &strLength));
    addon_data->endTime = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[10], addon_data->endTime, strLength, &strLength));

    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[11],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_set_city_datetime,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_set_city_datetime,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_set_city_datetime,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
/*********************************getCityDst**********************************************************/
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
    char* strCityId;
} AddonData_get_city_dst;

 napi_value get_city_dst_sync(napi_env env, napi_callback_info info) {
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //��ȡ����
    size_t strLength;
    //strCityId
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], NULL, NULL, &strLength));
    char* strCityId = (char*)malloc((++strLength) * sizeof(char));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], strCityId, strLength, &strLength));
    //ִ��so����
    char* result = getCityDst(strCityId);
    //ת����
    napi_value world;
    catch_error_get_datetime(env, napi_create_string_utf8(env, result, NAPI_AUTO_LENGTH, &world));
    //�ص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[1], &valueTypeLast);
    if (valueTypeLast == napi_function) {
        napi_value* callbackParams = &world;
        size_t callbackArgc = 1;
        napi_value global;
        napi_get_global(env, &global);
        napi_value callbackRs;
        napi_call_function(env, global, argv[1], callbackArgc, callbackParams, &callbackRs);
    }
    free(strCityId);
    return world;
}

void work_complete_get_city_dst(napi_env env, napi_status status, void* data) {
    AddonData_get_city_dst* addon_data = (AddonData_get_city_dst*)data;

    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release
    ));

    //��������
    catch_error_get_datetime(env, napi_delete_async_work(
        env, addon_data->work
    ));

    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    // Linux���������ͷŵ�ַ��������⣨�ڻص������н�û�а취��ȡ�����ֵ��������Window��û������
    free(addon_data->strCityId);
    free(addon_data);
}

void call_js_callback_get_city_dst(napi_env env, napi_value js_cb, void* context, void* data) {
    if (env != NULL)
    {
        // ��ȡ�첽�̷߳��صĽ��
        napi_value callBackInParams;
        catch_error_get_datetime(env, napi_create_string_utf8(
            env,
            (const char*)data,
            NAPI_AUTO_LENGTH,
            &callBackInParams
        ));

        //��ȡglobal
        napi_value undefined;
        catch_error_get_datetime(env, napi_get_undefined(env, &undefined));

        //���ú���
        catch_error_get_datetime(env, napi_call_function(
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
/**
 * ִ���߳�
*/
 void execute_work_get_city_dst(napi_env env, void* data)
{
    // ��������Ĳ���
    AddonData_get_city_dst* addon_data = (AddonData_get_city_dst*)data;
    //��ȡjs-callback����
    catch_error_get_datetime(env, napi_acquire_threadsafe_function(addon_data->tsfn));
    //ִ��so����
    char* result = getCityDst(addon_data->strCityId);
    char* rs = (char*)malloc(sizeof(char) * (strlen(result) + 1));
    strcpy(rs, result);
    // ����js-callback����
    catch_error_get_datetime(env, napi_call_threadsafe_function(
        addon_data->tsfn,                       // js-callback����
        rs,                    // call_js_callback�ĵ��ĸ�����
        //"Async Callback Result",
        napi_tsfn_blocking             // ������ģʽ����
    ));
    //�ͷž��
    catch_error_get_datetime(env, napi_release_threadsafe_function(
        addon_data->tsfn,
        napi_tsfn_release       //napi_tsfn_releaseָʾ��ǰ�̲߳��ٵ����̰߳�ȫ������napi_tsfn_abortָʾ����ǰ�߳��⣬�����̲߳�Ӧ�ٵ����̰߳�ȫ����
    ));

}

 napi_value get_city_dst(napi_env env, napi_callback_info info) {
    // ���߳��������
    napi_value work_name;
    //��ȡ����
    size_t argc = 2;
    napi_value argv[2];
    catch_error_get_datetime(env, napi_get_cb_info(
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
    //�ж��Ƿ���ڻص�����
    napi_valuetype valueTypeLast;
    napi_typeof(env, argv[1], &valueTypeLast);

    if (valueTypeLast != napi_function) {
        napi_throw_type_error(env, NULL, "Wrong arguments");
        return NULL;
    }
    // �����߳�����
    catch_error_get_datetime(env, napi_create_string_utf8(
        env,
        "N-API Thread-safe Get Performancen Work Item",
        NAPI_AUTO_LENGTH,
        &work_name
    ));
    //��ȡ����
    size_t strLength;
    AddonData_get_city_dst* addon_data = (AddonData_get_city_dst*)malloc(sizeof(*addon_data));
    addon_data->work = NULL;
    addon_data->tsfn = NULL;
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], NULL, NULL, &strLength));
    addon_data->strCityId = (char*)malloc(sizeof(char) * (++strLength));
    catch_error_get_datetime(env, napi_get_value_string_utf8(env, argv[0], addon_data->strCityId, strLength, &strLength));
    // �����ڴ�ռ䣬��work_complete�л��ͷ�
    // ��js������������̶߳�����ִ�еĺ���
    // �����Ϳ����ڿ����������߳��е�������
    // napi_create_threadsafe_function�������ã�
    // ����һ��napi_value�־����ã���ֵ�������ԴӶ���̵߳��õ�JS�������������첽�ģ�����ζ�ŵ���
    // JS�ص����õ�ֵ���������ڶ����У����ڶ����е�ÿ��ֵ�����ս�����JS����
    catch_error_get_datetime(env, napi_create_threadsafe_function(
        env,
        argv[1],   //JS����
        NULL,
        work_name,
        1,
        1,
        NULL,
        NULL,
        NULL,
        call_js_callback_get_city_dst,
        &(addon_data->tsfn)   //����napi_create_threadsafe_function�������첽��ȫJS����
    ));
    // ����ִ�����洴���ĺ���
   // napi_create_async_work�������ã�
    // ���������첽ִ���߼��Ĺ�������
    catch_error_get_datetime(env, napi_create_async_work(
        env,
        NULL,
        work_name,
        execute_work_get_city_dst,   // ��������ִ���첽�߼��ı��غ����������ĺ����Ǵӹ����ص��ã������¼�ѭ���̲߳���ִ��
        work_complete_get_city_dst,  // ���첽�߼���ɻ�ȡ��ʱ�����ã������¼�ѭ���߳��е���
        addon_data,          // �û��ṩ�����������ġ��⽫�����ݻ�execute��complete����
        &(addon_data->work)
    ));
    // ���̷߳ŵ���ִ�ж�����
    catch_error_get_datetime(env, napi_queue_async_work(
        env,
        addon_data->work   //Ҫִ���̵߳ľ��
    ));
    return NULL;
}
//
///****************************init***************************************/
//napi_value init(napi_env env, napi_value exports)
//{
//    /******************************getDatetimeSync***********************************************************/
//    // ��ȡ��������
//    napi_property_descriptor getDatetimeSync = {
//        "getDatetimeSync",
//        NULL,
//        get_datetime_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getDatetimeSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor getDatetime = {
//        "getDatetime",
//        NULL,
//        get_datetime,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getDatetime
//    );
//    /******************************getTimezone***********************************************************/
//    // ��ȡ��������
//    napi_property_descriptor getTimezoneSync = {
//        "getTimezoneSync",
//        NULL,
//        get_timezone_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getTimezoneSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor getTimezone = {
//        "getTimezone",
//        NULL,
//        get_timezone,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getTimezone
//    );
//    /******************************getDaylightSaving***********************************************************/
//   // ��ȡ��������
//    napi_property_descriptor getDaylightSavingSync = {
//        "getDaylightSavingSync",
//        NULL,
//        get_daylight_saving_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getDaylightSavingSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor getDaylightSaving = {
//        "getDaylightSaving",
//        NULL,
//        get_daylight_saving,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getDaylightSaving
//    );
//    /******************************getCityDatetime***********************************************************/
//   // ��ȡ��������
//    napi_property_descriptor getCityDatetimeSync = {
//        "getCityDatetimeSync",
//        NULL,
//        get_city_datetime_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getCityDatetimeSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor getCityDatetime = {
//        "getCityDatetime",
//        NULL,
//        get_city_datetime,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getCityDatetime
//    );
//    /******************************setDatetime***********************************************************/
//  // ��ȡ��������
//    napi_property_descriptor setDatetimeSync = {
//        "setDatetimeSync",
//        NULL,
//        set_datetime_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setDatetimeSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor setDatetime = {
//        "setDatetime",
//        NULL,
//        set_datetime,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setDatetime
//    );
//    /******************************setTimezone***********************************************************/
//// ��ȡ��������
//    napi_property_descriptor setTimezoneSync = {
//        "setTimezoneSync",
//        NULL,
//        set_timezone_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setTimezoneSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor setTimezone = {
//        "setTimezone",
//        NULL,
//        set_timezone,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setTimezone
//    );
//    /******************************setDaylightSaving***********************************************************/
//// ��ȡ��������
//    napi_property_descriptor setDaylightSavingSync = {
//        "setDaylightSavingSync",
//        NULL,
//        set_daylight_saving_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setDaylightSavingSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor setDaylightSaving = {
//        "setDaylightSaving",
//        NULL,
//        set_daylight_saving,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setDaylightSaving
//    );
//    /******************************setCityDatetime***********************************************************/
//    // ��ȡ��������
//    napi_property_descriptor setCityDatetimeSync = {
//        "setCityDatetimeSync",
//        NULL,
//        set_city_datetime_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setCityDatetimeSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor setCityDatetime = {
//        "setCityDatetime",
//        NULL,
//        set_city_datetime,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &setCityDatetime
//    );
//    /******************************getCityDst***********************************************************/
//// ��ȡ��������
//    napi_property_descriptor getCityDstSync = {
//        "getCityDstSync",
//        NULL,
//        get_city_dst_sync,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getCityDstSync
//    );
//
//    // ��ȡ��������
//    napi_property_descriptor getCityDst = {
//        "getCityDst",
//        NULL,
//        get_city_dst,
//        NULL,
//        NULL,
//        NULL,
//        napi_default,
//        NULL
//    };
//    //��¶�ӿ�
//    napi_define_properties(
//        env,
//        exports,
//        1,
//        &getCityDst
//    );
//    return exports;
//}
//
//NAPI_MODULE(NODE_GYP_MODULE_NAME, init);
