idf_component_register(SRCS "pka_aws.c"
                    INCLUDE_DIRS "." "include" "$ENV{IDF_PATH}/components/mqtt/esp-mqtt/include"
                    PRIV_REQUIRES  mqtt)

                    