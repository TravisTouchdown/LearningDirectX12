#include <CameraController.h>

#include <Camera.h>

#include <cmath>  // for std::abs and std::pow

using namespace DirectX;

// Perform a linear interpolation
inline double Lerp( float x0, float x1, float a )
{
    return x0 + a * ( x1 - x0 );
}

// Apply smoothing
inline void Smooth( float& x0, float& x1, float deltaTime )
{
    float x;
    if ( std::fabsf( x0 ) < std::fabsf( x1 ) )  // Speeding up
    {
        x = Lerp( x1, x0, std::powf( 0.6, deltaTime * 60.0 ) );
    }
    else  // Slowing down
    {
        x = Lerp( x1, x0, std::powf( 0.8, deltaTime * 60.0 ) );
    }

    x0 = x;
    x1 = x;
}

CameraController::CameraController( Camera& camera )
: m_Camera( camera )
, m_X( 0.0 )
, m_Y( 1.0 )
, m_Z( 0.0 )
, m_Pitch( 0.0 )
, m_Yaw( 0.0 )
, m_PreviousPitch( 0.0 )
, m_PreviousYaw( 0.0 )
, m_InverseY( true )
{
    auto& gf = GameFramework::Get();

    m_Logger = gf.CreateLogger( "CameraController" );

    m_KMInput  = gf.CreateInputMap( "CameraController (Keyboard/Mouse)" );
    m_PadInput = gf.CreateInputMap( "CameraController (Pad)" );

    auto keyboard = gf.GetKeyboardId();
    auto mouse    = gf.GetMouseId();
    auto pad      = gf.GetPadId( 0 );  // Just use the first connected device.

    // Map keyboard events.
    m_KMInput->MapFloat( MoveX, keyboard, gainput::KeyD, 0.0f, 1.0f );
    m_KMInput->MapFloat( MoveX, keyboard, gainput::KeyA, 0.0f, -1.0f );
    m_KMInput->MapFloat( MoveY, keyboard, gainput::KeyE, 0.0f, 1.0f );
    m_KMInput->MapFloat( MoveY, keyboard, gainput::KeyQ, 0.0f, -1.0f );
    m_KMInput->MapFloat( MoveZ, keyboard, gainput::KeyW, 0.0f, 1.0f );
    m_KMInput->MapFloat( MoveZ, keyboard, gainput::KeyS, 0.0f, -1.0f );
    m_KMInput->MapFloat( Pitch, keyboard, gainput::KeyUp, 0.0f, 1.0f );
    m_KMInput->MapFloat( Pitch, keyboard, gainput::KeyDown, 0.0f, -1.0f );
    m_KMInput->MapFloat( Yaw, keyboard, gainput::KeyLeft, 0.0f, 1.0f );
    m_KMInput->MapFloat( Yaw, keyboard, gainput::KeyRight, 0.0f, -1.0f );
    m_KMInput->MapBool( Boost, keyboard, gainput::KeyShiftL );
    m_KMInput->MapBool( Boost, keyboard, gainput::KeyShiftR );

    // Map mouse events
    m_KMInput->MapBool( LMB, mouse, gainput::MouseButtonLeft );
    m_KMInput->MapBool( RMB, mouse, gainput::MouseButtonRight );
    m_KMInput->MapFloat( Pitch, mouse, gainput::MouseAxisY );
    m_KMInput->MapFloat( Yaw, mouse, gainput::MouseAxisX );

    // Map pad events.
    m_PadInput->MapFloat( MoveX, pad, gainput::PadButtonLeftStickX );
    m_PadInput->MapFloat( MoveZ, pad, gainput::PadButtonLeftStickY );
    m_PadInput->MapFloat( MoveY, pad, gainput::PadButtonAxis4, 0.0f, -1.0f );  // Left trigger (move down)
    m_PadInput->MapFloat( MoveY, pad, gainput::PadButtonAxis5, 0.0f, 1.0f );   // Right trigger (move up)
    m_PadInput->MapFloat( Pitch, pad, gainput::PadButtonRightStickY );
    m_PadInput->MapFloat( Yaw, pad, gainput::PadButtonRightStickX );
    m_PadInput->MapBool( Boost, pad, gainput::PadButtonL3 );
    m_PadInput->MapBool( Boost, pad, gainput::PadButtonR3 );

    // Set policy for pitch/yaw so both mouse and keyboard works.
    m_KMInput->SetUserButtonPolicy( Pitch, gainput::InputMap::UBP_MAX );
    m_KMInput->SetUserButtonPolicy( Yaw, gainput::InputMap::UBP_MAX );

    ResetView();
}

void CameraController::ResetView() 
{
    // Reset previous deltas.
    m_X = m_Y = m_Z = m_PreviousPitch = m_PreviousYaw = 0.0f;
    m_Pitch         = 0.0f;
    m_Yaw           = 90.0f;

    XMVECTOR rotation =
        XMQuaternionRotationRollPitchYaw( XMConvertToRadians( m_Pitch ), XMConvertToRadians( m_Yaw ), 0.0f );
    m_Camera.set_Rotation( rotation );
    m_Camera.set_Translation( { 0, 1.5, 0.25, 1 } );

}

void CameraController::Update( UpdateEventArgs& e )
{
    const float  MOVE_SPEED        = 10.0;
    const float  LOOK_SENSITIVITY  = 180.0;
    const float  MOUSE_SENSITIVITY = 0.1;

    float  speedScale    = m_PadInput->GetBool( Boost ) || m_KMInput->GetBool( Boost ) ? 1.0 : 0.1;
    float rotationScale = m_PadInput->GetBool( Boost ) || m_KMInput->GetBool( Boost ) ? 1.0 : 0.5;

    float  X = ( m_KMInput->GetFloat( MoveX ) + m_PadInput->GetFloat( MoveX ) ) * MOVE_SPEED * speedScale * e.DeltaTime;
    float  Y = ( m_KMInput->GetFloat( MoveY ) + m_PadInput->GetFloat( MoveY ) ) * MOVE_SPEED * speedScale * e.DeltaTime;
    float  Z = ( m_KMInput->GetFloat( MoveZ ) + m_PadInput->GetFloat( MoveZ ) ) * MOVE_SPEED * speedScale * e.DeltaTime;
    float  pitch = m_PadInput->GetFloat( Pitch ) * LOOK_SENSITIVITY * rotationScale * e.DeltaTime;
    float  yaw   = m_PadInput->GetFloat( Yaw ) * LOOK_SENSITIVITY * rotationScale * e.DeltaTime;

    // Apply smoothing
    Smooth( m_X, X, e.DeltaTime );
    Smooth( m_Y, Y, e.DeltaTime );
    Smooth( m_Z, Z, e.DeltaTime );
    Smooth( m_PreviousPitch, pitch, e.DeltaTime );
    Smooth( m_PreviousYaw, yaw, e.DeltaTime );

    // Add mouse motion without smoothing.
    if ( m_KMInput->GetBool( LMB ) )
    {
        pitch -= m_KMInput->GetFloatDelta( Pitch ) * MOUSE_SENSITIVITY * rotationScale;
        yaw -= m_KMInput->GetFloatDelta( Yaw ) * MOUSE_SENSITIVITY * rotationScale;
    }

    m_Pitch += pitch * ( m_InverseY ? 1.0 : -1.0 );

    m_Pitch = std::clamp( m_Pitch, -90.0f, 90.0f );

    m_Yaw += yaw;

    // Apply translation and rotation to the camera.
    XMVECTORF32 translation = { X, Y, Z };
    m_Camera.Translate( translation );

    // Apply rotation
    XMVECTOR rotation =
        XMQuaternionRotationRollPitchYaw( XMConvertToRadians( m_Pitch ), XMConvertToRadians( m_Yaw ), 0.0f );
    m_Camera.set_Rotation( rotation );
}
