#pragma once

namespace core {

    struct IOResource
    {
        virtual ~IOResource() {}
    };

    typedef std::shared_ptr<IOResource> IOResourcePtr;

}
