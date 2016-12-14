"""
                 print len(header)
                 print "\ncontent length : "
                 print len(content)
                 headers=read_headers(header)
                 
                 if 'Transfer-Encoding' in headers:
                     if headers['Transfer-Encoding'].find('chunked')>0:  
                         
                         decode_data=decode_chunked(content)
                         print "======decode data=====\n\n"
                         
                       
                         modified_content=decode_data.replace(oldword, newword)
                         del headers['Transfer-Encoding']
 
                         headers['Content-Length']=str(len(modified_content)).encode()
                         
                 else:
                     modified_content=content.replace(oldword, newword)
                     headers['Content-Length']=str(len(modified_content)).encode()
 
                 modified_data=unite(headers, modified_content)
"""