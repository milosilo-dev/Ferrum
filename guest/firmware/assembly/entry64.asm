bits 64
global entry64
extern c_main_64

entry64:
    call c_main_64
    hlt