# IPPPrint

Printing from Ruby. This is a C wrapper for the CUPS IPP API.

## Installing

In your Gemfile `gem 'ipp_print', github: 'Unact/ipp_print'`

CUPS library needs to be installed on your system  
On MacOS - `brew install cups`  
On Debian or similar - `sudo apt-get install libcups2-dev`  

## Usage

Print something  

```ruby
printer = IPPPrint::Printer.new("127.0.0.1", 631, timeout: 30_000)
format = "text/plain"

Tempfile.open do |tempfile|
    tempfile.write("Hello world!")
    tempfile.rewind

    printer.print(tempfile, format)
end
```

Get printer status  

```ruby
printer = IPPPrint::Printer.new("127.0.0.1", 631, timeout: 30_000)

printer.info
```

Get job status  

```ruby
printer = IPPPrint::Printer.new("127.0.0.1", 631, timeout: 30_000)
job_uri = "some uri"

printer.job_info(job_uri)
```
