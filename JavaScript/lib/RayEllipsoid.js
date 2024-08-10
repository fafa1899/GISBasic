const epsilon = 0.0000000001;

//求解一元二次方程组ax*x + b*x + c = 0
function SolvingQuadratics(a, b, c) {
    let x = new Array();

    const delta = b * b - 4 * a * c;
    if (delta < 0) {
        return x;
    }

    //if (Math.abs(delta) < epsilon) {
    if (delta === 0) {
        x.push(-b / (2 * a));
    }
    else {
        x.push((-b + Math.sqrt(delta)) / (2 * a));
        x.push((-b - Math.sqrt(delta)) / (2 * a));
        x.sort(function (m, n) {
            return m - n; // 升序排序  
        });
    }
    return x;
}

function RayEllipsoid(halfAxis, rayOrigin, rayDirection) {
    const A = (rayDirection.x * rayDirection.x) / (halfAxis.x * halfAxis.x) +
        (rayDirection.y * rayDirection.y) / (halfAxis.y * halfAxis.y) +
        (rayDirection.z * rayDirection.z) / (halfAxis.z * halfAxis.z);
    const B = (2 * rayOrigin.x * rayDirection.x) / (halfAxis.x * halfAxis.x) +
        (2 * rayOrigin.y * rayDirection.y) / (halfAxis.y * halfAxis.y) +
        (2 * rayOrigin.z * rayDirection.z) / (halfAxis.z * halfAxis.z);
    const C = (rayOrigin.x * rayOrigin.x) / (halfAxis.x * halfAxis.x) +
        (rayOrigin.y * rayOrigin.y) / (halfAxis.y * halfAxis.y) +
        (rayOrigin.z * rayOrigin.z) / (halfAxis.z * halfAxis.z) - 1;

    let t = SolvingQuadratics(A, B, C);
    let result = new Array();
    for (let i = 0; i < t.length; i++) {
        let P = rayDirection.clone();
        P.multiplyScalar(t[i]);
        P.add(rayOrigin);
        result.push(P);
    }
    return result;
}

export { RayEllipsoid };

