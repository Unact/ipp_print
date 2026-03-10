#include <ipp_print.h>

extern VALUE mIPPPrint, cIPPPrintAttribute, cIPPPrintError;
static VALUE cIPPPrintPrinter;

// int(10 chars) + 1 char + int(10 chars) + 1 char + 4 char + NULL(1char)
#define MAX_RESOLUTION 27
#define DEFAULT_TIMEOUT 30000
#define DEFAULT_PROTOCOL "ipp"

#define GET_CONNECTION(self) \
  ipp_print_printer_wrapper *wrapper; \
  TypedData_Get_Struct(self, ipp_print_printer_wrapper, &ipp_print_printer_type, wrapper);

#define SEND_REQUEST(self, wrapper, data) \
  int cancel = 0; \
  data.http = wrapper->http; \
  data.cancel = &cancel; \
  data.timeout = wrapper->timeout;\
  \
  (void) rb_thread_call_without_gvl(nogvl_do_io_request, &data, nogvl_do_io_request_ubf, &data); \
  \
  if (data.error_number >= IPP_STATUS_ERROR_BAD_REQUEST) { \
    if (data.response) ippDelete(data.response); \
    rb_exc_raise(rb_ipp_printer_error(data.error_number, data.error_string)); \
  } \
  \
  VALUE info = rb_ipp_printer_response_to_attributes(data.response); \
  \
  ippDelete(data.response); \
  \
  return info;

struct nogvl_do_io_request_data {
  ipp_t *request;
  http_t *http;
  int timeout;
  int fd;

  int *cancel;

  int error_number;
  const char *error_string;
  ipp_t *response;
};

static void *nogvl_do_io_request(void *ptr) {
  struct nogvl_do_io_request_data *args = ptr;

  if (httpReconnect2(args->http, args->timeout, args->cancel) != 0) {
    args->error_number = cupsLastError();
    args->error_string = cupsLastErrorString();
    args->response = NULL;

    return NULL;
  }

  ipp_t *response = cupsDoIORequest(args->http, args->request, "/ipp/print", args->fd, -1);

  args->error_number = cupsLastError();
  args->error_string = cupsLastErrorString();
  args->response = response;

  return NULL;
}

static void nogvl_do_io_request_ubf(void *ptr) {
  struct nogvl_do_io_request_data *args = ptr;

  *args->cancel = 1;
}

static void ipp_print_printer_free(void *ptr) {
  ipp_print_printer_wrapper *wrapper = ptr;

  if (wrapper->http) {
    httpClose(wrapper->http);
  }

  free(wrapper);
}

static size_t ipp_print_printer_size(const void *ptr) {
  const ipp_print_printer_wrapper *wrapper = ptr;
  return sizeof(*wrapper);
}

static const rb_data_type_t ipp_print_printer_type = {
  .wrap_struct_name = "ipp_print_printer",
  .function = {
    .dmark = NULL,
    .dfree = ipp_print_printer_free,
    .dsize = ipp_print_printer_size,
  },
  .data = NULL,
  .flags = RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE rb_ipp_print_printer_allocate(VALUE self) {
  ipp_print_printer_wrapper *wrapper = malloc(sizeof *wrapper);

  wrapper->timeout = 0;
  wrapper->http = NULL;

  return TypedData_Wrap_Struct(self, &ipp_print_printer_type, wrapper);
}

static VALUE rb_ipp_printer_response_to_attributes(ipp_t *response) {
  VALUE attributes = rb_ary_new();

  for (ipp_attribute_t *attr = ippFirstAttribute(response); attr != NULL; attr = ippNextAttribute(response)) {
    VALUE attribute_value;
    VALUE attribute_value_arr = rb_ary_new();
    int count = ippGetCount(attr);
    const char *name = ippGetName(attr);

    for (int i = 0; i < count; i++) {
      switch(ippGetValueTag(attr)) {
      case IPP_TAG_RANGE:
        {
          int upper;
          int lower = ippGetRange(attr, i, &upper);

          attribute_value = rb_range_new(INT2NUM(lower), INT2NUM(upper), 0);
        }
        break;
      case IPP_TAG_INTEGER:
        attribute_value = INT2NUM(ippGetInteger(attr, i));
        break;
      case IPP_TAG_DATE:
        attribute_value = rb_time_new(ippDateToTime(ippGetDate(attr, i)), 0);
        break;
      case IPP_TAG_RESOLUTION:
        {
          char resolution[MAX_RESOLUTION];
          ipp_res_t units;
          int yres;
          int xres = ippGetResolution(attr, i, &yres, &units);

          snprintf(
            &resolution[0],
            sizeof(resolution),
            "%dx%d %s",
            xres,
            yres,
            units == IPP_RES_PER_INCH ? "dpi" : "dpcm"
          );

          attribute_value = rb_str_new_cstr(&resolution[0]);
        }
        break;
      case IPP_TAG_BOOLEAN:
        attribute_value = ippGetBoolean(attr, i) == 1 ? Qtrue : Qfalse;
        break;
      case IPP_TAG_ENUM:
        attribute_value = rb_str_new_cstr(ippEnumString(ippGetName(attr), ippGetInteger(attr, i)));
        break;
      case IPP_TAG_STRING:
        {
          int len;
          const char *octet = ippGetOctetString(attr, i, &len);

          attribute_value = rb_str_new(octet, len);
        }
        break;
      case IPP_TAG_TEXTLANG:
      case IPP_TAG_NAMELANG:
      case IPP_TAG_TEXT:
      case IPP_TAG_NAME:
      case IPP_TAG_KEYWORD:
      case IPP_TAG_URI:
      case IPP_TAG_URISCHEME:
      case IPP_TAG_CHARSET:
      case IPP_TAG_LANGUAGE:
      case IPP_TAG_MIMETYPE:
        attribute_value = rb_str_new_cstr(ippGetString(attr, i, NULL));
        break;
      case IPP_TAG_BEGIN_COLLECTION:
        attribute_value = rb_ipp_printer_response_to_attributes(ippGetCollection(attr, i));
        break;
      default:
        attribute_value = Qnil;
        break;
      }

      rb_ary_push(attribute_value_arr, attribute_value);
    }

    VALUE attribute = count > 1 ?
      rb_funcall(cIPPPrintAttribute, rb_intern("new"), 2, rb_str_new2(name), attribute_value_arr) :
      rb_funcall(cIPPPrintAttribute, rb_intern("new"), 2, rb_str_new2(name), rb_ary_pop(attribute_value_arr));

    rb_ary_push(attributes, attribute);
  }

  return attributes;
}

static VALUE rb_ipp_printer_error(int error_number, const char *error_string) {
  VALUE rb_error_string = rb_str_new2(error_string);

  rb_enc_associate(rb_error_string, rb_utf8_encoding());

  return rb_funcall(cIPPPrintError, rb_intern("new"), 2, rb_error_string, INT2NUM(error_number));
}

static VALUE rb_ipp_print_printer_info(VALUE self) {
  GET_CONNECTION(self);

  ipp_t *request = ippNewRequest(IPP_OP_GET_PRINTER_ATTRIBUTES);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, wrapper->uri);

  struct nogvl_do_io_request_data data;
  data.request = request;
  data.fd = -1;

  SEND_REQUEST(self, wrapper, data);
}

static VALUE rb_ipp_print_printer_job_info(VALUE self, VALUE rb_job_uri) {
  GET_CONNECTION(self);

  char *job_uri = StringValueCStr(rb_job_uri);

  ipp_t *request = ippNewRequest(IPP_OP_GET_JOB_ATTRIBUTES);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "job-uri", NULL, job_uri);

  struct nogvl_do_io_request_data data;
  data.request = request;
  data.fd = -1;

  SEND_REQUEST(self, wrapper, data);
}

static VALUE rb_ipp_print_printer_print(int argc, VALUE *argv, VALUE self) {
  GET_CONNECTION(self);

  ID keyword_ids[] = { rb_intern("job_name"), rb_intern("user_name") };
  VALUE rb_io;
  VALUE rb_format;
  VALUE opts;
  VALUE values[2];

  char *format;
  char *job_name;
  char *user_name;
  int fd;

  rb_scan_args(argc, argv, "2:", &rb_io, &rb_format, &opts);
  rb_get_kwargs(opts, keyword_ids, 0, 2, values);

  job_name = values[0] == Qundef ? NULL : StringValueCStr(values[0]);
  user_name = values[1] == Qundef ? NULL : StringValueCStr(values[1]);
  format = StringValueCStr(rb_format);
  fd = rb_io_descriptor(rb_io);

  ipp_t *request = ippNewRequest(IPP_OP_PRINT_JOB);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_URI, "printer-uri", NULL, wrapper->uri);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "job-name", NULL, job_name);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_NAME, "requesting-user-name", NULL, user_name);
  ippAddString(request, IPP_TAG_OPERATION, IPP_TAG_MIMETYPE, "document-format", NULL, format);

  struct nogvl_do_io_request_data data;
  data.request = request;
  data.fd = fd;

  SEND_REQUEST(self, wrapper, data);
}

static VALUE rb_ipp_print_printer_initialize(int argc, VALUE *argv, VALUE self) {
  GET_CONNECTION(self);

  ID keyword_ids[] = { rb_intern("timeout"), rb_intern("protocol") };
  VALUE rb_address;
  VALUE rb_port;
  VALUE opts;
  VALUE values[2];

  const char *c_address;
  int c_timeout;
  const char *c_protocol;
  int c_port;

  rb_scan_args(argc, argv, "2:", &rb_address, &rb_port, &opts);
  rb_get_kwargs(opts, keyword_ids, 0, 2, values);

  c_timeout = values[0] == Qundef ? DEFAULT_TIMEOUT : NUM2INT(values[0]);
  c_protocol = values[1] == Qundef ? DEFAULT_PROTOCOL : rb_id2name(SYM2ID(values[1]));
  c_address = StringValueCStr(rb_address);
  c_port = NUM2INT(rb_port);

  snprintf(&wrapper->uri[0], sizeof(wrapper->uri), "%s://%s:%i", c_protocol, c_address, c_port);
  wrapper->timeout = c_timeout;
  wrapper->http = httpConnect2(c_address, c_port, NULL, AF_UNSPEC, HTTP_ENCRYPTION_IF_REQUESTED, 1, 0, 0);

  return self;
}

void init_ipp_print_printer() {
  cIPPPrintPrinter = rb_define_class_under(mIPPPrint, "Printer", rb_cObject);

  rb_define_alloc_func(cIPPPrintPrinter, rb_ipp_print_printer_allocate);
  rb_define_method(cIPPPrintPrinter, "initialize", rb_ipp_print_printer_initialize, -1);
  rb_define_method(cIPPPrintPrinter, "print", rb_ipp_print_printer_print, -1);
  rb_define_method(cIPPPrintPrinter, "info", rb_ipp_print_printer_info, 0);
  rb_define_method(cIPPPrintPrinter, "job_info", rb_ipp_print_printer_job_info, 1);
}
