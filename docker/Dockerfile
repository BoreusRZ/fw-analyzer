FROM archlinux as build
RUN pacman -Sy --noconfirm git cmake ninja fmt gcc benchmark openmp
RUN git clone https://gitlab.boreus.de/alex/fw-analyzer.git --single-branch --depth 1
RUN cd fw-analyzer\
&& mkdir build\
&& cd build\
&& cmake -GNinja -DCMAKE_BUILD_TYPE=Release ..\
&& ninja analyzer

FROM archlinux 
RUN pacman -Sy --noconfirm fmt openmp gcc-libs
COPY --from=build /fw-analyzer/build/analyzer bin/analyzer
