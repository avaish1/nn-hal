/*
 * Copyright (C) 2017 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define LOG_TAG "NNHAL"

#include "Driver.h"
#include "PreparedModel.h"
#include <android-base/logging.h>
#include <thread>

namespace android {
namespace hardware {
namespace neuralnetworks {
namespace nnhal {

static sp<PreparedModel> ModelFactory(const char* name, const Model& model) {
    ALOGI("Entering %s",__func__);

	sp<PreparedModel> preparedModel = NULL;
    if (strcmp(name, "CPU") ==0)
        preparedModel = new CpuPreparedModel(model);
    else if (strcmp(name, "VPU") ==0)
        preparedModel = new VpuPreparedModel(model);

    return preparedModel;
}

Return<ErrorStatus> Driver::prepareModel(const V10_Model& model,
                                             const sp<IPreparedModelCallback>& callback)
{
	ALOGI("Entering %s",__func__);

	return ErrorStatus::NONE;
}

Return<ErrorStatus> Driver::prepareModel_1_1(const Model& model,ExecutionPreference preference, const sp<IPreparedModelCallback>& callback)
{
    ALOGI("Entering %s",__func__);
	
    if (callback.get() == nullptr) {
        ALOGI("invalid callback passed to prepareModel");
        return ErrorStatus::INVALID_ARGUMENT;
    }
	
	if(preference != ExecutionPreference::LOW_POWER && preference != ExecutionPreference::FAST_SINGLE_ANSWER &&
	   preference != ExecutionPreference::SUSTAINED_SPEED){
		ALOGI("NNERR:Execution preference not valid,aborting!! ") ;
		callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
	}
	
   

    // TODO: make asynchronous later
   	sp<PreparedModel> preparedModel = ModelFactory(mName.c_str(), model);
	
    if (preparedModel == NULL) {
        ALOGI("failed to create preparedmodel");
        return ErrorStatus::INVALID_ARGUMENT;
    }
	
	if (!PreparedModel::validModel(model)) {
        ALOGI("model is not valid,mModelmutate=%d",PreparedModel::mModelmutate) ;
		if ( PreparedModel::mModelmutate == true) {
			    ALOGI("NNERR:Model seems corrupted for some operation even, resetting now ");
				PreparedModel::mModelmutate = false ;//resetting
	}		
        callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
        return ErrorStatus::INVALID_ARGUMENT;
    }
		
    if (!preparedModel->initialize()) {
        ALOGI("failed to initialize preparedmodel");
		if (PreparedModel::mModelmutate == true) {
			    ALOGI("NNERR:Model seems corrupted for some operation even, resetting now ");
				PreparedModel::mModelmutate = false ;//resetting
				callback->notify(ErrorStatus::INVALID_ARGUMENT, nullptr);
				return ErrorStatus::INVALID_ARGUMENT;
		}
		callback->notify(ErrorStatus::GENERAL_FAILURE, nullptr);
		return ErrorStatus::NONE;
        
    }
	
    callback->notify(ErrorStatus::NONE, preparedModel);
    return ErrorStatus::NONE;
}

Return<DeviceStatus> Driver::getStatus() {
    ALOGI("Entering %s",__func__);

    return DeviceStatus::AVAILABLE;
}

Return<void> Driver::getCapabilities(getCapabilities_cb cb) {
	//V10_Capabilities capabilities = {.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f}};
	//cb(ErrorStatus::NONE, capabilities);
	ALOGI("Entering %s",__func__);

	return Void();
}

Return<void> Driver::getCapabilities_1_1(getCapabilities_1_1_cb cb) {
    ALOGI("Entering %s",__func__);
	if (mName.compare("CPU") == 0) {
        ALOGI("Cpu driver getCapabilities()");
        Capabilities capabilities = {.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f},
											  .quantized8Performance = {.execTime = 0.9f, .powerUsage = 0.9f},
											  .relaxedFloat32toFloat16Performance = {.execTime = 0.9f, .powerUsage = 0.9f}};
        cb(ErrorStatus::NONE, capabilities);
        ALOGI(".execTime = 0.9f, .powerUsage = 0.9f with float32,Quantized and Relaxed Performance data");
    } else { /* mName.compare("VPU") == 0 */
        ALOGI("Myriad driver getCapabilities()");
        //OMR1 V1_0

        Capabilities capabilities = {.float32Performance = {.execTime = 1.1f, .powerUsage = 1.1f},
                                 .quantized8Performance = {.execTime = 1.1f, .powerUsage = 1.1f}};

        //Capabilities capabilities = {.float32Performance = {.execTime = 0.9f, .powerUsage = 0.9f}};
        ALOGI("Myriad driver Capabilities .execTime = 1.1f, .powerUsage = 1.1f");
        //P MR0 V1_1
        /*
        Capabilities capabilities = {.float32Performance = {.execTime = 1.1f, .powerUsage = 1.1f},
                                    .quantized8Performance = {.execTime = 1.1f, .powerUsage = 1.1f},
                                    .relaxedFloat32toFloat16Performance =
                                        {.execTime = 1.1f, .powerUsage = 1.1f}};
        */

        cb(ErrorStatus::NONE, capabilities);
    }
    return Void();
}
Return<void> Driver::getSupportedOperations(const V10_Model& model,getSupportedOperations_cb cb) {
    //std::vector<bool> supported(count, true);
    //cb(ErrorStatus::NONE, supported);
    ALOGI("Entering %s",__func__);

	return Void();												 
}

Return<void> Driver::getSupportedOperations_1_1(const Model& model,
                                                     getSupportedOperations_1_1_cb cb) {
    //VLOG(DRIVER) << "getSupportedOperations()";
    ALOGI("Entering %s",__func__);

    int count = model.operations.size();
	std::vector<bool> supported(count, true);
	
    if (!PreparedModel::validModel(model)) {
		
        ALOGI("NNERR-model is not valid,returned invalid argument,aborting!!");
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
    }
	
	for (int i = 0; i < count; i++) {
        const auto& operation = model.operations[i];
		//ALOGI("Checking Operation Support for Index %d",i);
        supported[i] = PreparedModel::isOperationSupported(operation, model);
    }
	 
	if ( PreparedModel::mModelmutate == true) {
		ALOGI("NNERR:Model Corrupted ,aborting!! ");
		PreparedModel::mModelmutate = false; //resetting
        cb(ErrorStatus::INVALID_ARGUMENT, supported);
        return Void();
	}
    cb(ErrorStatus::NONE, supported);
    return Void();
}

}  // namespace nnhal
}  // namespace neuralnetworks
}  // namespace hardware
}  // namespace android 
