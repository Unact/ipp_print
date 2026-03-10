#include <ipp_print.h>

VALUE mIPPPrint, cIPPPrintAttribute, cIPPPrintError;

void Init_ipp_print() {
  mIPPPrint = rb_define_module("IPPPrint");
  cIPPPrintError = rb_const_get(mIPPPrint, rb_intern("Error"));
  cIPPPrintAttribute = rb_const_get(mIPPPrint, rb_intern("Attribute"));

  init_ipp_print_printer();
}
