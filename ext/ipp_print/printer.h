#ifndef IPP_PRINT_PRINTER_H
#define IPP_PRINT_PRINTER_H

typedef struct {
  http_t *http;
  char uri[IPP_MAX_URI];
  int timeout;
} ipp_print_printer_wrapper;

void init_ipp_print_printer(void);

#endif
