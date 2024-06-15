//
// Created by 李子健 on 2023/7/7.
//

#ifndef NS3_MPI_DATATYPE_H
#define NS3_MPI_DATATYPE_H

namespace ns3 {
    struct FLOAT_INT{
        float f;
        int i;
    };

    struct DOUBLE_INT{
        double d;
        int i;
    };

    struct REPEATED_INT{
        int i_1;
        int i_2;
    };

    struct LONG_DOUBLE_INT{
        long double ld;
        int i;
    };
}

#endif // NS3_MPI_DATATYPE_H
