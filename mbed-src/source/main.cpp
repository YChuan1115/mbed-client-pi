/*
 * Copyright (c) 2015 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <unistd.h>
#include <pthread.h>
#include <stdio.h>
#include <time.h>
#include <stdlib.h>
#include <signal.h> /* For SIGIGN and SIGINT */
#include "mbed-client/m2minterfacefactory.h"
#include "mbed-client/m2mdevice.h"
#include "mbed-client/m2minterfaceobserver.h"
#include "mbed-client/m2minterface.h"
#include "mbed-client/m2mobjectinstance.h"
#include "mbed-client/m2mresource.h"
#include "security.h"
#include "mbed-trace/mbed_trace.h"
#include <string>


#include <stdint.h>
#include "bcm2835.h"
#include "bme280.h"
#include "lib_i2c.h"
#include "bme280_i2c/include/prin_bme280.h"

#include "oled_i2c/include/my_debug.h"
#include "oled_i2c/include/I2C_SSD1306Z.h"	

#define DEVICE_NAME "ittraining-joseph"
#define BME280_ADDR 0x76


#define LCD_I2C_SLA   0x3c

#define i2c_trigger 0



//create M2M socket
//Select binding mode: UDP or TCP
M2MInterface::BindingMode SOCKET_MODE = M2MInterface::UDP;
const String &MBED_USER_NAME_DOMAIN = MBED_DOMAIN;
const String &ENDPOINT_NAME = MBED_ENDPOINT_NAME;


//create resource
// This is address to mbed Device Connector
const String &MBED_SERVER_ADDRESS = "coap://api.connector.mbed.com:5684";
const String &MANUFACTURER = "manufacturer";
const String &TYPE = "type";
const String &MODEL_NUMBER = "2015";
const String &SERIAL_NUMBER = "12345";

const uint8_t value[] = "MyValue";
const uint8_t STATIC_VALUE[] = "Static value";

static void ctrl_c_handle_function(void);
typedef void (*signalhandler_t)(int); /* Function pointer type for ctrl-c */
pthread_t unregister_thread;
pthread_t observation_thread;
pthread_t update_register_thread;
pthread_t button_thread;

volatile bool ctlC = false;
volatile bool loop;
int b23 = 0, b24 = 0;
unsigned int earseOLED = 0;
unsigned int overflowOLED = 0;
unsigned int buzzctl = 0; 
 
//buffer stores information of post request
char postOnOLED[20] = {0};

struct bme280_t *mbedbme;


class MbedClient: public M2MInterfaceObserver {
public:
    MbedClient(){
        _security = NULL;
        _interface = NULL;
        _device = NULL;
        _object = NULL;
		_object_gpio = NULL;
        _error = false;
        _registered = false;
        _unregistered = false;
        _registration_updated = false;
		
    }

    ~MbedClient() {
        if(_security) {
            delete _security;
        }
        if( _register_security){
            delete _register_security;
        }
        if(_device) {
            M2MDevice::delete_instance();
        }
        if(_object) {
            delete _object;
		}
		if(_object_gpio) {
            delete _object_gpio;
		}
        if(_interface) {
            delete _interface;
        }
    }

    bool create_interface() {

       time_t t;
       srand((unsigned) time(&t));

       uint16_t port = rand() % 65535 + 12345;

       _interface = M2MInterfaceFactory::create_interface(*this,
                                                  ENDPOINT_NAME,
                                                  DEVICE_NAME,
                                                  100,
                                                  port,
                                                  MBED_USER_NAME_DOMAIN,
                                                  SOCKET_MODE,
                                                  M2MInterface::LwIP_IPv4,
                                                  "");
        printf("Endpoint Name : %s\n", ENDPOINT_NAME.c_str());
        return (_interface == NULL) ? false : true;
    }

    bool register_successful() { 
	 //沒有成功但也沒有錯誤 就先睡1秒再作判斷
        while(!_registered && !_error) {
            sleep(1);
        }
        return _registered;
    }

    bool unregister_successful() {
        while(!_unregistered && !_error) {
            sleep(1);
        }
        return _unregistered;
    }

    bool registration_update_successful() {
        while(!_registration_updated && !_error) {
        }
        return _registration_updated;
    }

    bool create_register_object() {
        bool success = false;
        _register_security = M2MInterfaceFactory::create_security(M2MSecurity::M2MServer);
        if(_register_security) {
            _register_security->set_resource_value(M2MSecurity::M2MServerUri, MBED_SERVER_ADDRESS);
            _register_security->set_resource_value(M2MSecurity::SecurityMode, M2MSecurity::Certificate);
            _register_security->set_resource_value(M2MSecurity::ServerPublicKey,SERVER_CERT,sizeof(SERVER_CERT));
            _register_security->set_resource_value(M2MSecurity::PublicKey,CERT,sizeof(CERT));
            _register_security->set_resource_value(M2MSecurity::Secretkey,KEY,sizeof(KEY));
            success = true;
        }
        return success;
    }

    bool create_device_object() {
        bool success = false;
        _device = M2MInterfaceFactory::create_device();
        if(_device) {
            if(_device->create_resource(M2MDevice::Manufacturer,MANUFACTURER)     &&
               _device->create_resource(M2MDevice::DeviceType,TYPE)        &&
               _device->create_resource(M2MDevice::ModelNumber,MODEL_NUMBER)      &&
               _device->create_resource(M2MDevice::SerialNumber,SERIAL_NUMBER)) {
                success = true;
            }
        }
        return success;
    }
	
    void execute_function_gpio(void *argument) {
        if(argument) {  //fetch the payload of the POST
            M2MResource::M2MExecuteParameter* param = (M2MResource::M2MExecuteParameter*)argument;
            String object_name = param->get_argument_object_name();
            uint16_t object_instance_id = param->get_argument_object_instance_id();
            String resource_name = param->get_argument_resource_name();
            int payload_length = param->get_argument_value_length();
            uint8_t* payload = param->get_argument_value();
            printf("Resource: %s/%d/%s executed\n", object_name.c_str(), object_instance_id, resource_name.c_str());
            printf("Payload: %.*s\n", payload_length, payload);
			sprintf(postOnOLED, "%s/%d/%s:%s", object_name.c_str(), object_instance_id, resource_name.c_str(), payload);
			
			char cmd[20] = {0};
			int  pin, arg;
			char *set_pin = NULL, *value = NULL;
			
			
			sprintf(cmd, "%s", payload);
			if(cmd != NULL || ' '){
				if(cmd[0] > '9' || cmd[0] < '0')
					printf("Invalid Command\n");
				else{
					
					set_pin = strtok(cmd, "%/");
					value = strtok(NULL, "%/");
					
					if(value == NULL || set_pin == NULL)
						printf("Invalid Command\n");					
					
					else{
						pin = atoi(set_pin);
						arg = atoi(value);
						switch(pin){
							
							case 22:
								bcm2835_gpio_fsel(22, BCM2835_GPIO_FSEL_OUTP);
								bcm2835_gpio_write(22, arg);
								break;
							case 27:
								bcm2835_gpio_fsel(27, BCM2835_GPIO_FSEL_OUTP);
								bcm2835_gpio_write(27, arg);
								break;
							case 12:
								if(arg >= 1){						
									bcm2835_gpio_fsel(12, BCM2835_GPIO_FSEL_OUTP);
									buzzctl = 1;
								}else if(arg == 0){
									bcm2835_gpio_fsel(12, BCM2835_GPIO_FSEL_INPT);
									buzzctl = 0;
								}
								break;
							case 17:
								if(arg == 1){
									bcm2835_gpio_fsel(17, BCM2835_GPIO_FSEL_OUTP);
									bcm2835_gpio_write(17, arg);
								}else if(arg ==0){
									bcm2835_gpio_fsel(17, BCM2835_GPIO_FSEL_OUTP);
									bcm2835_gpio_write(17, arg);
									
								}
								break;
							
							default:
								printf("Invalid Command\n");
						}
					}
				}
			}					
        }
    }
	
#if i2c_trigger	

	void execute_function(void *argument) {
        if(argument) {  //fetch the payload of the POST
            M2MResource::M2MExecuteParameter* param = (M2MResource::M2MExecuteParameter*)argument;
            String object_name = param->get_argument_object_name();
            uint16_t object_instance_id = param->get_argument_object_instance_id();
            String resource_name = param->get_argument_resource_name();
            int payload_length = param->get_argument_value_length();
            uint8_t* payload = param->get_argument_value();
            printf("Resource: %s/%d/%s executed\n", object_name.c_str(), object_instance_id, resource_name.c_str());
            printf("Payload: %.*s\n", payload_length, payload);
			sprintf(postOnOLED, "%s/%d/%s:%s", object_name.c_str(), object_instance_id, resource_name.c_str(), payload);
					
        }
    }
//Here to change the resources
    bool create_generic_object() {
        bool success = false;
        _object = M2MInterfaceFactory::create_object("Monitor");
        if(_object) {
            M2MObjectInstance* inst = _object->create_object_instance();
            if(inst) {
                inst->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
                inst->set_register_uri(true);
                M2MResource* res = inst->create_dynamic_resource("D",
                                                                 "ResourceTest",
                                                                 M2MResourceInstance::FLOAT,
                                                                 true);
                char buffer[20];
                 bme280_force_read_measurement(mbedbme);
                int size = sprintf(buffer,"%6.2f", bme280_get_temperature_most_accurate(mbedbme));
                  printf("Temp: %s\n",buffer);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  res->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  res->set_value((const uint8_t*)buffer,
                                 (const uint32_t)size);
				 //for dynamic "resource" itself, we can set the execution callback whenever mDS send the POST request to us
                   res->set_execute_function(execute_callback(this,&MbedClient::execute_function));
              /*  inst->create_static_resource("S",
                                             "ResourceTest",
                                             M2MResourceInstance::STRING,
                                             STATIC_VALUE,
                                             sizeof(STATIC_VALUE)-1);*/
                                             
              M2MResource* reshp = inst->create_dynamic_resource("Hp",
                                                               "ResourceTest",
                                                                M2MResourceInstance::FLOAT,
                                                                true);
                char bufferhp[20];
                bme280_force_read_measurement(mbedbme);
                int sizehp = sprintf(bufferhp,"%6.2f", bme280_get_pressure(mbedbme));
                  printf("Pressure: %s\n",bufferhp);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  reshp->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  reshp->set_value((const uint8_t*)bufferhp,
                                 (const uint32_t)sizehp);
				 //for dynamic "resource" itself, we can set the execution callback whenever mDS send the POST request to us
                   reshp->set_execute_function(execute_callback(this,&MbedClient::execute_function));
              /*  inst->create_static_resource("S",
                                             "ResourceTest",
                                             M2MResourceInstance::STRING,
                                             STATIC_VALUE,
                                             sizeof(STATIC_VALUE)-1);*/
              M2MResource* resh = inst->create_dynamic_resource("Hum",
                                                               "ResourceTest",
                                                                M2MResourceInstance::FLOAT,
                                                                true);
                char bufferh[20];
                bme280_force_read_measurement(mbedbme);
                int sizeh = sprintf(bufferh,"%6.2f", bme280_get_humidity_most_accurate(mbedbme));
                printf("Humidity: %s\n",bufferh);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  resh->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  resh->set_value((const uint8_t*)bufferh,
                                 (const uint32_t)sizeh);
				 //for dynamic "resource" itself, we can set the execution callback whenever mDS send the POST request to us
                   resh->set_execute_function(execute_callback(this,&MbedClient::execute_function));
              /*  inst->create_static_resource("S",
                                             "ResourceTest",
                                             M2MResourceInstance::STRING,
                                             STATIC_VALUE,
                                             sizeof(STATIC_VALUE)-1);*/
            }
        }
        return success;
    }
#endif		
	bool create_generic_object_gpio() {
        bool success = false;	
		_object_gpio = M2MInterfaceFactory::create_object("GPIO");
        if(_object_gpio) {
		
			M2MObjectInstance* inst_gpio_1 = _object_gpio->create_object_instance(0);
            if(inst_gpio_1) {
                inst_gpio_1->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
                inst_gpio_1->set_register_uri(true);								 
                M2MResource* gpio_state = inst_gpio_1->create_dynamic_resource("STATE",
                                                                 "GPIOSTATE",
                                                                 M2MResourceInstance::STRING,
                                                                 true);
                char buffer_state[256];
				int state_state = 0;
                int size_state = sprintf(buffer_state,"{\"GPIO_STATUS\":[{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d}]}"
													 ,12,buzzctl,17,bcm2835_gpio_lev(17),22,bcm2835_gpio_lev(22),23,!(bcm2835_gpio_lev(23)),24,!(bcm2835_gpio_lev(24)),27,bcm2835_gpio_lev(27));																							
                  printf("%s\n",buffer_state);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  gpio_state->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  gpio_state->set_value((const uint8_t*)buffer_state,
                                       (const uint32_t)size_state);
				 //for dynamic "resource" itself, we can set the execution callback whenever mDS send the POST request to us
                   gpio_state->set_execute_function(execute_callback(this,&MbedClient::execute_function_gpio));
				   
				M2MResource* gpio_23 = inst_gpio_1->create_dynamic_resource("Button23",
                                                                 "GPIOSTATE",
                                                                 M2MResourceInstance::INTEGER,
                                                                 true);
                char buffer_button23[20];
                int size23 = sprintf(buffer_button23,"Button Count23 = %d", b23);																								
                printf("%s\n",buffer_button23);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  gpio_23->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  gpio_23->set_value((const uint8_t*)buffer_button23,
                                       (const uint32_t)size23);
				
				 M2MResource* gpio_24 = inst_gpio_1->create_dynamic_resource("Button24",
                                                                 "GPIOSTATE",
                                                                 M2MResourceInstance::INTEGER,
                                                                 true);
                char buffer_button24[20];
                int size24 = sprintf(buffer_button24,"Button Count24 = %d", b24);																								
                printf("%s\n",buffer_button24);
				//set the operation mode of the Objects so that they can handle GET, PUT, POST, DELETE
                  gpio_24->set_operation(M2MBase::GET_PUT_POST_DELETE_ALLOWED);
				  // send GET command to mDS server
                  gpio_24->set_value((const uint8_t*)buffer_button24,
                                       (const uint32_t)size24);
			}                             
		
            
        }
        return success;
    }
	
    void update_resource() {

#if i2c_trigger
        if(_object) {
            M2MObjectInstance* inst = _object->object_instance();
            if(inst) {
                M2MResource* res = inst->resource("D");
                           
                char buffer[20]={0};
                bme280_force_read_measurement(mbedbme);
                int size = sprintf(buffer,"%6.2f",bme280_get_temperature_most_accurate(mbedbme));
                printf("%s",buffer);
                //res->set_value() means send resource to mDS by GET requesst 
           
                res->set_value((const uint8_t*)buffer,
                               (const uint32_t)size);
                
                M2MResource* reshp = inst->resource("Hp");
                           
                char bufferhp[20]={0};
                bme280_force_read_measurement(mbedbme);
                int sizehp = sprintf(bufferhp,"%6.2f",bme280_get_pressure(mbedbme));
                printf("%s",bufferhp);
                //res->set_value() means send resource to mDS by GET requesst 
                
                reshp->set_value((const uint8_t*)bufferhp,
                                 (const uint32_t)sizehp);
                
                M2MResource* resh = inst->resource("Hum");
                           
                char bufferh[20]={0};
                bme280_force_read_measurement(mbedbme);
                int sizeh = sprintf(bufferh,"%6.2f",bme280_get_humidity_most_accurate(mbedbme));
                printf("%s",bufferh);
                //res->set_value() means send resource to mDS by GET requesst 
              
                resh->set_value((const uint8_t*)bufferh,
                               (const uint32_t)sizeh);
							   
				char tbuffer[50], hpbuffer[50],hbuffer[50];
				
				sprintf(tbuffer, "Temp: %s C", buffer);
				sprintf(hpbuffer, "P: %s Hp", bufferhp);
				sprintf(hbuffer, "Hum: %s %%", bufferh);
				
				tbuffer[strlen(tbuffer)] = '\0';
				hpbuffer[strlen(hpbuffer)] = '\0';
				hbuffer[strlen(hbuffer)] = '\0';
				
				print_Line(0, tbuffer);
				print_Line(1, hpbuffer); 
				print_Line(2, hbuffer);
				// When fetch post request, print information on bottom of OLED and earse it after 4 times update.

				if(strlen(postOnOLED) > 16){
					char *path = NULL;
					char *argm = NULL;
					char local[20] = {0};
					strcpy(local, postOnOLED);
					path = strtok(local, ":");
					argm = strtok(NULL, ":");
					path[strlen(path)] = '\0';
					if(overflowOLED <= 3){
						print_Line(3, path);
						overflowOLED++;
					}
					else if(overflowOLED > 3 && overflowOLED <= 6){
						clear_LCD_line(3);
						print_Line(3, argm);
						overflowOLED++;
					}
					else{
						clear_LCD_line(3);
						overflowOLED = 0;
						for (int i = 0; i < 20; i++)
							postOnOLED[i] = 0;
					}
					
				}else{
					if(earseOLED  > 4){
						clear_LCD_line(3);
						for (int i = 0; i < 20; i++)
							postOnOLED[i] = 0;
						earseOLED = 0;
					}
					else if(postOnOLED[0] != 0){
						print_Line(3, postOnOLED);
						earseOLED++;
					}
				}
			}
        }
    
#endif		
		if(_object_gpio) {
			M2MObjectInstance* inst_gpio_1 = _object_gpio->object_instance();
            if(inst_gpio_1) {                							 
                M2MResource* gpio_state = inst_gpio_1->resource("STATE");
                char buffer_state[256];
				int state_state = 0;
                int size_state = sprintf(buffer_state,"{\"GPIO_STATUS\":[{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d},{\"GPIO\":%d,\"value\":%d}]}"
													 ,12,buzzctl,17,bcm2835_gpio_lev(17),22,bcm2835_gpio_lev(22),23,!(bcm2835_gpio_lev(23)),24,!(bcm2835_gpio_lev(24)),27,bcm2835_gpio_lev(27));																								
                  printf("%s\n",buffer_state);
                  gpio_state->set_value((const uint8_t*)buffer_state,
                                        (const uint32_t)size_state);								

			}                             
			
			M2MObjectInstance* inst_gpio = _object_gpio->object_instance();
            if(inst_gpio_1) {                							 
                M2MResource* gpio_23 = inst_gpio->resource("Button23");
				
                char buffer_button23[20];
                int size23 = sprintf(buffer_button23,"Button Count23 = %d", b23);				
                printf("%s\n",buffer_button23);																							
                gpio_23->set_value((const uint8_t*)buffer_button23,
                                       (const uint32_t)size23);
						
				M2MResource* gpio_24 = inst_gpio->resource("Button24");
				
                char buffer_button24[20];
                int size24 = sprintf(buffer_button24,"Button Count24 = %d", b24);				
                printf("%s\n",buffer_button24);																							
                gpio_24->set_value((const uint8_t*)buffer_button24,
                                       (const uint32_t)size24);

			}                      
		
		}
    }

    void register_endpoint(){
        M2MObjectList object_list;
        object_list.push_back(_device);
#if i2c_trigger        
		object_list.push_back(_object);
#endif
		object_list.push_back(_object_gpio);
		
    if(_interface) {// to register endpoint to  mDS server
            _interface->register_object(_register_security,object_list);
        } else {
        printf("Interface doesn't exist, exiting!!\n");
            exit(1);
    }
    }

    void test_update_register() {
        uint32_t updated_lifetime = 100;  //send update registeration, periodically
        if(_interface) {
            _interface->update_registration(_register_security,updated_lifetime);
        }
    }

    void mbedClient_unregister() {
		ctlC = true;
        if(_interface) {
			//Deregister: call M2MInterface::unregister_object(M2MSecurity *object);
            _interface->unregister_object(NULL);  // when only one mDS , pass NULL
        }
    }

    void bootstrap_done(M2MSecurity */*server_object*/){
    }
    // callback function when register successfully
    void object_registered(M2MSecurity */*security_object*/, const M2MServer &/*server_object*/){
        _registered = true;
        printf("\nRegistered\n");
		clear_LCD();
		strcpy(postOnOLED, "RegisteringDone");
		print_Line(3, postOnOLED);
    }
    // callback function when unregister successfully
    void object_unregistered(M2MSecurity */*server_object*/){
        _unregistered = true;
        printf("\nUnregistered\n");
    }
    // callback function when update registration information successfully
    void registration_updated(M2MSecurity */*security_object*/, const M2MServer & /*server_object*/){
        _registration_updated = true;
        printf("\nregistration updated\n");

    }
    // callback function when some errors occur  
    void error(M2MInterface::Error error){
        _error = true;
        String error_code;
	switch(error) {
        case M2MInterface::ErrorNone:
             error_code += "M2MInterface::ErrorNone";
			 mbedClient_unregister();
             break;
            case M2MInterface::AlreadyExists:
             error_code += "M2MInterface::AlreadyExists";
			 mbedClient_unregister();
             break;
            case M2MInterface::BootstrapFailed:
             error_code += "M2MInterface::BootstrapFailed";
			 mbedClient_unregister();
             break;
            case M2MInterface::InvalidParameters:
             error_code += "M2MInterface::InvalidParameters";
			 mbedClient_unregister();
             break;
            case M2MInterface::NotRegistered:
             error_code += "M2MInterface::NotRegistered";
			 mbedClient_unregister();
             break;
            case M2MInterface::Timeout:
             error_code += "M2MInterface::Timeout";
			 mbedClient_unregister();
             break;
            case M2MInterface::NetworkError:
             error_code += "M2MInterface::NetworkError";
			 mbedClient_unregister();
             break;
            case M2MInterface::ResponseParseFailed:
             error_code += "M2MInterface::ResponseParseFailed";
			 mbedClient_unregister();
             break;
            case M2MInterface::UnknownError:
             error_code += "M2MInterface::UnknownError";
			 mbedClient_unregister();
             break;
            case M2MInterface::MemoryFail:
             error_code += "M2MInterface::MemoryFail";
			 mbedClient_unregister();
             break;
            case M2MInterface::NotAllowed:
             error_code += "M2MInterface::NotAllowed";
			 mbedClient_unregister();
             break;
            case M2MInterface::SecureConnectionFailed:
             error_code += "M2MInterface::SecureConnectionFailed";
			 mbedClient_unregister();
             break;
            case M2MInterface::DnsResolvingFailed:
             error_code += "M2MInterface::DnsResolvingFailed";
			 mbedClient_unregister();
             break;
        }
        printf("\nError occured  : %s\n", error_code.c_str());
    }
    //callback "value_updated" : Whenever there is a valid PUT operation for any of the resources,
    void value_updated(M2MBase *base, M2MBase::BaseType type) {
            M2MResource* resource = NULL;
            String object_name = "";
            String resource_name = "";
            uint16_t object_instance_id = 0;
            if(base) {
                switch(base->base_type()) {
                    case M2MBase::Resource: {
                        resource = (M2MResource*)base;
                        object_name = resource->object_name();
                        object_instance_id = resource->object_instance_id();
                        resource_name = resource->name();
                        printf("Resource: %s/%d/%s value updated\r\n",
                               resource->object_name().c_str(), resource->object_instance_id(), resource->name().c_str());
                    }
                    break;
                default:
                    break;
                }
            }
        }

	
		
		
private:

    M2MInterface        *_interface;
    M2MSecurity         *_security;
    M2MSecurity         *_register_security;
    M2MDevice           *_device;
    M2MObject           *_object;
	M2MObject           *_object_gpio;
    bool                _error;
    bool                _registered;
    bool                _unregistered;
    bool                _registration_updated;

};

void* wait_for_unregister(void* arg) {
    MbedClient *client;
    client = (MbedClient*) arg;
    if(client->unregister_successful()) {
        printf("Unregistered done\n");
        loop = false;
        pthread_detach(update_register_thread);
        pthread_detach(observation_thread);
        pthread_detach(unregister_thread);
		pthread_detach(button_thread);

        pthread_cancel(update_register_thread);
        pthread_cancel(observation_thread);
        pthread_cancel(unregister_thread);
		pthread_cancel(button_thread);
    }
    return NULL;
}




void* buttonobs(void* arg) {
	MbedClient *client;
    client = (MbedClient*) arg;
	while(1){
		sleep(1);
		if(bcm2835_gpio_lev(23) == 0){
			b23++;
		}else if(bcm2835_gpio_lev(24) == 0){		
			b24++;
		}	
	}

    return NULL;
}

void* send_observation(void* arg) {
    MbedClient *client;
    client = (MbedClient*) arg;
    int counter = 0;
    while(1) {
        usleep(500000);
        if(counter >= 1 &&
           client->register_successful() && !ctlC) {
            printf("Monitor Resource Value\n");
            client->update_resource();
            counter = 0;
        }
        else
            counter++;
    }
    return NULL;
}

void* update_register(void* arg) {
    MbedClient *client;
    client = (MbedClient*) arg;
    static uint8_t counter = 0;
    while(1) {
        sleep(50);
        if(client->register_successful() && !ctlC) {
            printf("Sending update register\n");
            client->test_update_register();
        }
    }
    return NULL;
}

static MbedClient *mbedclient = NULL;

static void ctrl_c_handle_function(void)
{
    if(mbedclient && mbedclient->register_successful()) {
		ctlC = true;
		sleep(1);
        printf("\nUnregistering endpoint and EXITING Program\n");
        mbedclient->mbedClient_unregister();
   }
	  loop = false;
}


void gpio_init(){
	
	bcm2835_gpio_fsel(12, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(22, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(27, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(24, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(23, BCM2835_GPIO_FSEL_INPT);
	bcm2835_gpio_fsel(17, BCM2835_GPIO_FSEL_INPT);
	

	
}


int main() {
	
	int renew = 5;
	loop = true;
    signal(SIGINT, (signalhandler_t)ctrl_c_handle_function);
    MbedClient mbed_client;

    mbedclient = &mbed_client;
    mbed_trace_init();

    //-- init the bcm2835 library
    bcm2835_init();
	
#if i2c_trigger	
	
    i2c_start();
	Init_LCD();
    clear_LCD();
    //-- new a bme280 device module with i2c_addr

    mbedbme = bme280_new(BME280_ADDR);   
	if(mbedbme == 0) return -1;
    bme280_initi(mbedbme);
    while (renew) {
	if (bme280_get_temperature(mbedbme) > 45 || bme280_get_humidity(mbedbme) == 0 || bme280_get_pressure(mbedbme) == 0){
		  bme280_write_mode(mbedbme,BME280_MODE_SLEEP);
          bme280_free(mbedbme);
          bcm2835_close();
		  sleep(1);
		  //re init bme280         
		  bcm2835_init();
          i2c_start();
		  mbedbme = bme280_new(BME280_ADDR);  
		  if(mbedbme == 0) return -1;
		  bme280_initi(mbedbme);
		  renew--;
	} else {
         printf("init bme280 successfully\n");
		 print_Line(0, "Initializing");
		 print_Line(1, "PeripheralModules");
		 print_Line(2, "Successfully");
		 sleep(1);
        break;		
	}
	
	}
	
#endif	
	gpio_init();
	
	printf("\n................Start mbed client .........................\n");
#if i2c_trigger	
	clear_LCD();
	print_Line(0, ".............");
	print_Line(1, "-Registering-");
	print_Line(2, ".............");
#endif		

	//-- init the bcm2835 library


    bool result = mbed_client.create_interface();  //crere socket of endpoint
    if(true == result) {
        printf("\nInterface created\n");
#if i2c_trigger
        print_Line(3,"InterfaceCreated");
#endif
    }
    result = mbed_client.create_register_object();  //crearte security object for interface
    if(true == result) {
        printf("register object created");
    }

    result = mbed_client.create_device_object();
    if(true == result){
        printf("\nDevice object created !!\n");
    }
#if i2c_trigger
    result = mbed_client.create_generic_object(); //create /object/object instance/resource

    if(true == result) {
        printf("\nGeneric object created\n");
    }
#endif	
	result = mbed_client.create_generic_object_gpio(); //create /object/object instance/resource

    if(true == result) {
        printf("\nGeneric object created\n");
    }

	
    printf("Registering endpoint\n");
#if i2c_trigger	
	clear_LCD();
	strcpy(postOnOLED, "..Registering..");
	print_Line(3, postOnOLED);
#endif   
    mbed_client.register_endpoint();
 
    pthread_create(&button_thread, NULL, &buttonobs, (void*) &mbed_client);
    pthread_create(&observation_thread, NULL, &send_observation, (void*) &mbed_client);
    pthread_create(&unregister_thread, NULL, &wait_for_unregister, (void*) &mbed_client);
    pthread_create(&update_register_thread, NULL, &update_register, (void*) &mbed_client);	   


    while(loop);
    bcm2835_gpio_fsel(12, BCM2835_GPIO_FSEL_INPT);
#if i2c_trigger
    bme280_write_mode(mbedbme,BME280_MODE_SLEEP);
    bme280_free(mbedbme);
#endif
	sleep(1);
	clear_LCD();
	print_Line(0, ".............");
	print_Line(1, "Unregistering");
	print_Line(2, ".............");
	print_Line(3, "Done");
	sleep(2);
	clear_LCD();
    bcm2835_close();
	
	
	
    return 0;
}

