

module.exports = function (dir) {
    return function (arg) {
        var paths = Array.prototype.slice.call(arguments);
        var org = Array.prototype.slice.call(arguments);

        var check_next = function () {
            var path = paths.shift();

            if(!path)
                throw new Error("Cannot find module " + org.join(", ") + ".");

            try {
                return require(path);
            } catch (e) {
                if (e.message.slice(0,18) !== "Cannot find module")
                    throw e;
                return check_next();
            }
        };

        return check_next();
    };
};

