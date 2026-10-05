import 'toolcool-color-picker';
import {useEffect, useRef} from "react";

declare global {
    namespace JSX {
        interface IntrinsicElements {
            'toolcool-color-picker': any;
        }
    }
}

interface ColorPickerProps
{
    onChangeColor: (rgb:string) => void;
}
const ColorPicker = ({onChangeColor}: ColorPickerProps) => {

    const colorPickerRef = useRef<HTMLElement>();

    useEffect(() => {

        const colorPicker = colorPickerRef.current;

        const onColorChange = (evt: Event) => {
            const customEvent = evt as CustomEvent;
            onChangeColor(customEvent.detail.rgb);
           // console.log(customEvent.detail.rgb);
        };

        colorPicker?.addEventListener('change', onColorChange);

        return () => {
            colorPicker?.removeEventListener('change', onColorChange);
        };
    }, [onChangeColor]);

    return (
        <button className={"btn btn-outline"}>
        <toolcool-color-picker ref={ colorPickerRef } color="#ff0000" />
        </button>
    )
};

export default ColorPicker;