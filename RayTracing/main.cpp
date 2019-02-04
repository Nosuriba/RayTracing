#include<dxlib.h>
#include<math.h>
#include<vector>
#include<limits>
#include<assert.h>
#include"Geometry.h"

const int screen_width = 640;
const int screen_height = 480;

Position3 spPos = { -150, 0, 0 };
Position3 spPos2 = { 150, 50, -50 };

Vector3 yellow = { 1, 1, 0.8f };
Vector3 red = { 1, 0.8, 0.8 };

Vector3 pNormal = { 0, 1, 0 };
Vector3 eye		= { 0, 0, 300 };
	
/// 一定範囲内の値を返す関数(レイトレで使用する成分用)
float Clamp(const float& value, const float& minVal = 0.0f, const float& maxVal = 1.0f)
{
	return max(minVal, min(maxVal, value));
}

/// 一定範囲内の値を返す関数(色の成分用)
Vector3 Clamp(const Vector3& value)
{
	return Vector3(Clamp(value.x), Clamp(value.y), Clamp(value.z));
}

/// 入射ベクトルと指定した法線の反射ベクトルを返す
Vector3 ReflectVector(const Vector3& inVec, const Vector3& normal)
{
	return inVec - normal * 2 * Dot(inVec, normal);
}

/// 床の色を取得するための関数
Vector3 GetPlaneColor(const Vector3& hitPos)
{
	// 平面模様の位置を調整するためのもの
	int sftPlane = (hitPos.x < 0 ? 1 : 0) + (hitPos.z < 0 ? 1 : 0);

	if (((int)(hitPos.x / 80) + (int)(hitPos.z / 80) + sftPlane) % 2)
	{
		return Vector3(0, 0.7f, 0.3f);
	}
	else
	{
		return Vector3(0.2f, 0.2f, 0.2f);
	}
}

/// レイトレで求めた成分を使って計算した色の成分を返すための関数(球体の色付け用)
unsigned int CalculateColor(const Vector3& albedo, const float& diffuse, const float& specular, const float& ambient)
{
	auto color = albedo;
	auto spcPos = Vector3(specular, specular, specular);
/// [colorの値が一定範囲を超えて鏡面反射が目玉みたいになっていた]
	color = Clamp((albedo * (diffuse * ambient) + spcPos));			

	return GetColor(0xff * color.x, 0xff * color.y, 0xff * color.z);
}

/// 床の色を取得するための関数
unsigned int PlaneColor(Vector3 color)
{
	return GetColor(0xff * color.x, 0xff * color.y, 0xff * color.z);
}

///レイ(光線)と球体の当たり判定
///@param ray (視点からスクリーンピクセルへのベクトル)
///@param sphere 球
///@hint レイは正規化しといたほうが使いやすいだろう
bool IsHitRayAndObject(const Position3& eye,const Vector3& ray,const Sphere& sp, float& distance) {

	//視点から球体中心へのベクトル(視線)を作ります[正規化するな!]
	auto eyeVec = sp.pos - eye;					// 球体中心までの視線ベクトル
	auto rayVec = ray * Dot(ray, eyeVec);		// 垂線までの視線ベクトル
	auto line = eyeVec - rayVec;				// レイの当たった位置から球体の中心までの垂線を求めている

	if (line.Magnitude() <= sp.radius)
	{
		auto sinLength = sqrt((sp.radius * sp.radius) - (line.Magnitude() * line.Magnitude()));
		distance = rayVec.Magnitude() - sinLength;
		return true;
	}
	
	return false;
}

///レイ(光線)と床の当たり判定
bool IsHitRayAndObject(const Position3& eye, const Vector3& ray, const Plane& plane, float& distance) {

	distance = (Dot(eye, plane.normal) - plane.offset) / Dot(ray, plane.normal);

	return (Dot(ray, plane.normal) > 0);
}

bool HitCheck(const Position3& eye, const Vector3& light, Vector3& ray,
	const Vector2& scrPos, const Sphere& sp, const Plane& plane, float& distance)
{
	if (IsHitRayAndObject(eye, ray, sp, distance))
	{
		assert(distance >= 0.0f);

		Position3 hitPos = eye + ray * distance;
		auto normal = hitPos - sp.pos;
		normal.Normalize();

		Vector3 albedo = sp.albedo;
		float k = 1.0;		// 鏡面反射の定数
		auto diffuse = 0.0f;
		auto specular = 0.0f;
		auto ambient = 1.0f;

		diffuse = Clamp(Dot(light, normal));

		auto reflectVec = ReflectVector(light, normal);
		reflectVec.Normalize();
		if (diffuse > 0.0f)
		{
			specular += k * pow(Dot(reflectVec, -ray), 20);
		}
		auto reflectRay = ReflectVector(ray, normal);
		/*reflectRay.Normalize();*/

		/// 交点座標からの反射ベクトルが床と当たったかの判定を行っている
		if (IsHitRayAndObject(hitPos, reflectRay, plane, distance))
		{
			/// レイが当たった床の位置の色を取得して、球体に色を付けている
			auto pos = hitPos + reflectRay * distance;
			//auto pos = hitPos - reflectRay * distance;
			DrawPixel(scrPos.x, scrPos.y, PlaneColor(GetPlaneColor(pos) * (diffuse + 0.2f)));
		}
		else
		{
			DrawPixel(scrPos.x, scrPos.y, CalculateColor(albedo, diffuse, specular, ambient));
		}
		return true;
	}
	return false;
}

///レイトレーシング
///@param eye 視点座標
///@param sphere 球オブジェクト(そのうち複数にする)
void RayTracing(const Position3& eye, const Plane& plane)
{
	Sphere sp1 = { 100, spPos, yellow };
	Sphere sp2 = { 100, spPos2, red };

	for (int y = 0; y < screen_height; ++y) {//スクリーン縦方向
		for (int x = 0; x < screen_width; ++x) {//スクリーン横方向
			Position3 screenPos( x - screen_width / 2, y - screen_height / 2, 0);
			Vector3 light = { -1, -1, 1 };
			light.Normalize();
			Vector3 ray = screenPos - eye;
			ray.Normalize();
			float distance = 0.0f;
			
			if (!HitCheck(eye, light, ray, Vector2(x, y), sp1, plane, distance) &&
				!HitCheck(eye, light, ray, Vector2(x, y), sp2, plane, distance))
			{
				if (IsHitRayAndObject(eye, ray, plane, distance))
				{
					auto hitPos = eye + ray * distance;
					auto color = PlaneColor(GetPlaneColor(hitPos));
					if (IsHitRayAndObject(hitPos, -light, sp1, distance))
					{
						color = PlaneColor(GetPlaneColor(hitPos) - Vector3(0, 0.2f, 0.2f));
					}
					else if (IsHitRayAndObject(hitPos, -light, sp2, distance))
					{
						color = PlaneColor(GetPlaneColor(hitPos) - Vector3(0, 0.2f, 0.2f));
					}
					DrawPixel(x, y, color);
				}
				else
				{
					if (((x * x / 60) + (y * y / 60)) % 2 & (x + y) % 2)
					{
						DrawPixel(x, y, 0x00a1e3);
					}
					else
					{
						DrawPixel(x, y, 0x000000);
					}
				}
			}
		}
	}
}

int main() {
	ChangeWindowMode(true);
	SetGraphMode(screen_width, screen_height, 32);
	SetMainWindowText(_T("1701310_北川 潤一"));
	DxLib_Init();
	while(!ProcessMessage() & !CheckHitKey(KEY_INPUT_ESCAPE))
	{
		ClsDrawScreen();
		/// 球体の移動
		if (CheckHitKey(KEY_INPUT_RIGHT))
		{
			spPos.x++;
		}
		if (CheckHitKey(KEY_INPUT_LEFT))
		{
			spPos.x--;
		}
		if (CheckHitKey(KEY_INPUT_DOWN))
		{
			spPos.y++;
		}
		else if (CheckHitKey(KEY_INPUT_LSHIFT) & CheckHitKey(KEY_INPUT_DOWN))
		{
			spPos.z++;
		}
		if (CheckHitKey(KEY_INPUT_UP))
		{
			spPos.y--;
		}
		else if (CheckHitKey(KEY_INPUT_LSHIFT) & CheckHitKey(KEY_INPUT_UP))
		{
			spPos.z--;
		}
		/// 床の位置を定数から変えておく
		RayTracing(eye, Plane(pNormal, -300));
		DxLib_End;
	}
}