
otpch.h.gch: otpch.h 
	$(CPP) -c otpch.h -o otpch.h.gch $(CXXFLAGS)

all-before: otpch.h.gch

clean-custom:
	${RM} otpch.h.gch
