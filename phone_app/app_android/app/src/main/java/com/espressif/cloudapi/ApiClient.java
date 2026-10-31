// Copyright 2020 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

package com.espressif.cloudapi;

import android.content.Context;

import com.espressif.AppConstants;
import com.espressif.rainmaker.BuildConfig;
import java.util.concurrent.TimeUnit;

import okhttp3.OkHttpClient;
import retrofit2.Retrofit;
import retrofit2.adapter.rxjava2.RxJava2CallAdapterFactory;
import retrofit2.converter.gson.GsonConverterFactory;

/**
 * This class represents Retrofit Client to call REST APIs.
 */
public class ApiClient {

    private static Retrofit retrofitClient = null;

    static Retrofit getClient(Context context) {

        // The bundled Amazon Root CA 1 expired in 2025. Use Android's current
        // system trust store for the RainMaker HTTPS endpoints instead.
        OkHttpClient okHttpClient = new OkHttpClient.Builder()
                .authenticator(new TokenAuthenticator(context))
                .connectTimeout(15, TimeUnit.SECONDS)
                .writeTimeout(15, TimeUnit.SECONDS)
                .readTimeout(15, TimeUnit.SECONDS)
                .build();

        retrofitClient = new Retrofit.Builder()
                .baseUrl(BuildConfig.BASE_URL + AppConstants.PATH_SEPARATOR + AppConstants.CURRENT_VERSION + AppConstants.PATH_SEPARATOR)
                .addConverterFactory(GsonConverterFactory.create())
                .addCallAdapterFactory(RxJava2CallAdapterFactory.create())
                .client(okHttpClient)
                .build();

        return retrofitClient;
    }
}
